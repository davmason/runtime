#include <eventpipe/ep-rt-config.h>

#ifdef ENABLE_PERFTRACING
#include <eventpipe/ep-types.h>
#include <eventpipe/ep.h>
#include <eventpipe/ep-stack-contents.h>
#include <eventpipe/ep-rt.h>
#include "threadsuspend.h"

ep_rt_lock_handle_t _ep_rt_coreclr_config_lock_handle;
CrstStatic _ep_rt_coreclr_config_lock;

thread_local EventPipeCoreCLRThreadHolderTLS EventPipeCoreCLRThreadHolderTLS::g_threadHolderTLS;

ep_char8_t *volatile _ep_rt_coreclr_diagnostics_cmd_line;

#ifndef TARGET_UNIX
uint32_t *_ep_rt_coreclr_proc_group_offsets;
#endif

/*
 * Forward declares of all static functions.
 */

static
StackWalkAction
stack_walk_callback (
	CrawlFrame *frame,
	EventPipeStackContents *stack_contents);

static
void
walk_managed_stack_for_threads (
	ep_rt_thread_handle_t sampling_thread,
	EventPipeEvent *sampling_event);

static MethodTable *pTaskMT = NULL;
static FieldDesc *pCurrentTaskFD = NULL;

static
StackWalkAction
stack_walk_callback (
	CrawlFrame *crawl_frame,
	EventPipeStackContents *stack_contents)
{
	STATIC_CONTRACT_NOTHROW;
	EP_ASSERT (crawl_frame != NULL);
	EP_ASSERT (stack_contents != NULL);

	// Get the IP.
	UINT_PTR control_pc = (UINT_PTR)crawl_frame->GetRegisterSet ()->ControlPC;
	if (control_pc == NULL) {
		if (ep_stack_contents_get_length (stack_contents) == 0) {
			// This happens for pinvoke stubs on the top of the stack.
			return SWA_CONTINUE;
		}
	}

	EP_ASSERT (control_pc != NULL);

	// Add the IP to the captured stack.
	ep_stack_contents_append (stack_contents, control_pc, crawl_frame->GetFunction());

	// Continue the stack walk.
	return SWA_CONTINUE;
}

bool
ep_rt_coreclr_walk_managed_stack_for_thread (
	ep_rt_thread_handle_t thread,
	EventPipeStackContents *stack_contents)
{
	STATIC_CONTRACT_NOTHROW;
	EP_ASSERT (thread != NULL);
	EP_ASSERT (stack_contents != NULL);

	// Calling into StackWalkFrames in preemptive mode violates the host contract,
	// but this contract is not used on CoreCLR.
	CONTRACT_VIOLATION (HostViolation);

	{
		GCX_COOP();

		if (pCurrentTaskFD != NULL)
		{
			TADDR pCurrentTask = thread->GetStaticFieldAddrNoCreate(pCurrentTaskFD);
			if (pCurrentTask != NULL)
			{
				TaskObject *task_obj = *((TaskObject **)(pCurrentTask));
				if (task_obj != NULL)
				{
					printf("Thread %d has current task %p\n", thread->GetOSThreadId(), task_obj);

					EP_ASSERT (task_obj->GetMethodTable()->IsEquivalentTo(pTaskMT)
						|| (task_obj->GetMethodTable()->GetMethodTableMatchingParentClass(pTaskMT) != NULL));

					int indent = 0;
					while(true)
					{
						task_obj = task_obj->GetParent();
						if (task_obj == NULL)
						{
							break;
						}

						++indent;
						for (int i = 0; i < indent * 4; ++i)
						{
							printf(" ");
						}

						printf("task=%p\n", task_obj);
					}
				}
			}
		}
	}

	// Before we call into StackWalkFrames we need to mark GC_ON_TRANSITIONS as FALSE
	// because under GCStress runs (GCStress=0x3), a GC will be triggered for every transition,
	// which will cause the GC to try to walk the stack while we are in the middle of walking the stack.
	bool gc_on_transitions = GC_ON_TRANSITIONS (FALSE);

	// printf("starting stackwalk for thread %d\n", thread->GetOSThreadId());
	StackWalkAction result = thread->StackWalkFrames (
		(PSTACKWALKFRAMESCALLBACK)stack_walk_callback,
		stack_contents,
		ALLOW_ASYNC_STACK_WALK | FUNCTIONSONLY | HANDLESKIPPEDFRAMES | ALLOW_INVALID_OBJECTS);
	// printf("done with stackwalk\n");

	GC_ON_TRANSITIONS (gc_on_transitions);
	return ((result == SWA_DONE) || (result == SWA_CONTINUE));
}

// The thread store lock must already be held by the thread before this function
// is called.  ThreadSuspend::SuspendEE acquires the thread store lock.
static
void
walk_managed_stack_for_threads (
	ep_rt_thread_handle_t sampling_thread,
	EventPipeEvent *sampling_event)
{
	STATIC_CONTRACT_NOTHROW;
	EP_ASSERT (sampling_thread != NULL);

	// TODO: hacky check that system classes are loaded
	if (pTaskMT == NULL && g_pThreadClass != NULL)
	{
		pTaskMT = CoreLibBinder::GetClass(CLASS__TASK);
		pCurrentTaskFD = CoreLibBinder::GetField(FIELD__TASK__CURRENT_TASK);
	}

	Thread *target_thread = NULL;

	EventPipeStackContents stack_contents;
	EventPipeStackContents *current_stack_contents;
	current_stack_contents = ep_stack_contents_init (&stack_contents);

	EP_ASSERT (current_stack_contents != NULL);

	// Iterate over all managed threads.
	// Assumes that the ThreadStoreLock is held because we've suspended all threads.
	while ((target_thread = ThreadStore::GetThreadList (target_thread)) != NULL) {
		ep_stack_contents_reset (current_stack_contents);

		// Walk the stack and write it out as an event.
		if (ep_rt_coreclr_walk_managed_stack_for_thread (target_thread, current_stack_contents) && !ep_stack_contents_is_empty (current_stack_contents)) {
			// Set the payload.  If the GC mode on suspension > 0, then the thread was in cooperative mode.
			// Even though there are some cases where this is not managed code, we assume it is managed code here.
			// If the GC mode on suspension == 0 then the thread was in preemptive mode, which we qualify as external here.
			uint32_t payload_data = target_thread->GetGCModeOnSuspension () ? EP_SAMPLE_PROFILER_SAMPLE_TYPE_MANAGED : EP_SAMPLE_PROFILER_SAMPLE_TYPE_EXTERNAL;

			// Write the sample.
			ep_write_sample_profile_event (
				sampling_thread,
				sampling_event,
				target_thread,
				current_stack_contents,
				(uint8_t *)&payload_data,
				sizeof (payload_data));
		}

		// Reset the GC mode.
		target_thread->ClearGCModeOnSuspension ();
	}

	ep_stack_contents_fini (current_stack_contents);
}

void
ep_rt_coreclr_sample_profiler_write_sampling_event_for_threads (
	ep_rt_thread_handle_t sampling_thread,
	EventPipeEvent *sampling_event)
{
	STATIC_CONTRACT_NOTHROW;
	EP_ASSERT (sampling_thread != NULL);

	// Check to see if we can suspend managed execution.
	if (ThreadSuspend::SysIsSuspendInProgress () || (ThreadSuspend::GetSuspensionThread () != 0))
		return;

	// Actually suspend managed execution.
	ThreadSuspend::SuspendEE (ThreadSuspend::SUSPEND_REASON::SUSPEND_OTHER);

	// Walk all managed threads and capture stacks.
	walk_managed_stack_for_threads (sampling_thread, sampling_event);

	// Resume managed execution.
	ThreadSuspend::RestartEE (FALSE /* bFinishedGC */, TRUE /* SuspendSucceeded */);

	return;
}

#endif /* ENABLE_PERFTRACING */
