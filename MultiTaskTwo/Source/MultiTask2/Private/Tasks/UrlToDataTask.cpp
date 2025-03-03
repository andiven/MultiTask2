// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#include "UrlToDataTask.h"
#include "HAL/UnrealMemory.h"
#include "Interfaces/IHttpResponse.h"
#include "MultiTaskThreadPool.h"
#ifndef ENGINE_MINOR_VERSION
#include "Runtime/Launch/Resources/Version.h"
#endif
#include "Modules/ModuleManager.h"
bool UUrlToDataTask::Start()
{

    bCanceled = false;

     UUrlToDataTask* Worker = this;

    EAsyncExecution AsyncType = EAsyncExecution::ThreadPool;
    switch (ExecutionType)
    {
    case ETaskExecutionType::TaskGraph:
        AsyncType = EAsyncExecution::TaskGraph;
        break;
    case ETaskExecutionType::Thread:
        AsyncType = EAsyncExecution::Thread;
        break;
    case ETaskExecutionType::ThreadPool:
        AsyncType = EAsyncExecution::ThreadPool;
        break;
    }

    TFunction<void()> BodyFunc = [Worker]()
    {
        if (IsValid(Worker) && !Worker->HasAnyFlags(RF_BeginDestroyed) && !Worker->IsUnreachable())
        {
            Worker->TaskBody();
        }
    };

    TFunction<void()> OnCompleteFunc = [Worker]()
    {
        AsyncTask(ENamedThreads::GameThread, [Worker]()
        {
            if (IsValid(Worker) && !Worker->HasAnyFlags(RF_BeginDestroyed) && !Worker->IsUnreachable())
            {
                if (!Worker->IsCanceled())
                {
                    Worker->OnComplete();
                }
            }
        });
    };

    Tasks.SetNumZeroed(1);
    if (AsyncType == EAsyncExecution::ThreadPool && ThreadPool && ThreadPool->GetThreadsNum() > 0)
    {
        Tasks[0] = AsyncPool(ThreadPool->Obj.ToSharedRef().Get(), TUniqueFunction<void()>(BodyFunc), TUniqueFunction<void()>(OnCompleteFunc));
    }
    else {
        Tasks[0] = Async(AsyncType, TUniqueFunction<void()>(BodyFunc), TUniqueFunction<void()>(OnCompleteFunc));
    }
    return true;
}

void UUrlToDataTask::Cancel()
{
    if (IsRunning() && !IsCanceled())
    {
        URLRequest->CancelRequest();
    }
    Super::Cancel();
}

void UUrlToDataTask::TaskBody_Implementation()
{
    UUrlToDataTask* Worker = this;
    auto OnResponseReceived = [Worker](FHttpRequestPtr Request, const FHttpResponsePtr Response, const bool bWasSuccessful)
    {
        if (IsValid(Worker) && !Worker->HasAnyFlags(RF_BeginDestroyed) && !Worker->IsUnreachable())
        {
            if (bWasSuccessful && Response.IsValid())
            {
                Worker->Data = Response->GetContent();
            }
            if (Worker->URLRequest.IsValid())
            {
                Worker->URLRequest.Reset();
            }

        }
    };

    URLRequest->OnProcessRequestComplete().BindLambda(OnResponseReceived);
    URLRequest->ProcessRequest();

}

bool UUrlToDataTask::IsRunning()
{
    return URLRequest.IsValid();
}
