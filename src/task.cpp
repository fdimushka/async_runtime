#include "ar/task.hpp"

using namespace AsyncRuntime;


void Task::SetWorkGroupExecutorState(const ObjectID &group)
{
    if (executor_state.work_group != group)
        executor_state.processor = INVALID_OBJECT_ID;

    executor_state.work_group = group;
}


void Task::SetOriginId(uintptr_t origin_id)
{
    origin_id_ = origin_id;
}


Timespan Task::GetDelay() const
{
    if(!delayed) return 0;
    return start_time - TIMESTAMP_NOW_MICRO();
}
