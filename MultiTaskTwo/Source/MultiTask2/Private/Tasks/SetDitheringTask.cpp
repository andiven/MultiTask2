// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#include "SetDitheringTask.h"
#include "MultiTaskThreadPool.h"
bool USetDitheringTask::Start()
{
    if (Scale <= 0)
    {
        return false;
    }

    if (!PixelData.IsValid())
    {
        return false;
    }

    bCanceled = false;

    USetDitheringTask* Worker = this;

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

void USetDitheringTask::TaskBody_Implementation()
{

    uint8 LocalScale = (uint8)FMath::Clamp(Scale, 1, 128);

    for (int32 Y = 1; Y < PixelData.GetHeight() - 1; Y++)
    {
        for (int32 X = 1; X < PixelData.GetWidth() - 1; X++)
        {
            if (IsCanceled())
            {
                return;
            }
            FColor Pixel;
            PixelData.GetPixel(FIntPoint(X, Y), Pixel);

            uint8 NewR = (Scale * Pixel.R / 255) * (255 / Scale);
            uint8 NewG = (Scale * Pixel.G / 255) * (255 / Scale);
            uint8 NewB = (Scale * Pixel.B / 255) * (255 / Scale);

            PixelData.SetPixel(FIntPoint(X, Y), FColor(NewR, NewG, NewB, Pixel.A));

            uint8 ErrorR = Pixel.R - NewR;
            uint8 ErrorG = Pixel.G - NewG;
            uint8 ErrorB = Pixel.B - NewB;

            PixelData.GetPixel(FIntPoint(X + 1, Y), Pixel);

            NewR = Pixel.R + ErrorR * (7.0f / 16.0f);
            NewG = Pixel.G + ErrorG * (7.0f / 16.0f);
            NewB = Pixel.B + ErrorB * (7.0f / 16.0f);

            PixelData.SetPixel(FIntPoint(X + 1, Y), FColor(NewR, NewG, NewB, Pixel.A));


            PixelData.GetPixel(FIntPoint(X - 1, Y + 1), Pixel);

            NewR = Pixel.R + ErrorR * (3.0f / 16.0f);
            NewG = Pixel.G + ErrorG * (3.0f / 16.0f);
            NewB = Pixel.B + ErrorB * (3.0f / 16.0f);

            PixelData.SetPixel(FIntPoint(X - 1, Y + 1), FColor(NewR, NewG, NewB, Pixel.A));


            PixelData.GetPixel(FIntPoint(X, Y + 1), Pixel);

            NewR = Pixel.R + ErrorR * (5.0f / 16.0f);
            NewG = Pixel.G + ErrorG * (5.0f / 16.0f);
            NewB = Pixel.B + ErrorB * (5.0f / 16.0f);

            PixelData.SetPixel(FIntPoint(X, Y + 1), FColor(NewR, NewG, NewB, Pixel.A));


            PixelData.GetPixel(FIntPoint(X + 1, Y + 1), Pixel);

            NewR = Pixel.R + ErrorR * (1.0f / 16.0f);
            NewG = Pixel.G + ErrorG * (1.0f / 16.0f);
            NewB = Pixel.B + ErrorB * (1.0f / 16.0f);

            PixelData.SetPixel(FIntPoint(X + 1, Y + 1), FColor(NewR, NewG, NewB, Pixel.A));

        }
    }
}