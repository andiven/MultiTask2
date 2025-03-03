// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "MultiThreadTask.h"
#include "PixelReaderTask.h"
#include "SetDitheringTask.generated.h"

UCLASS(HideDropdown, NotBlueprintable, NotBlueprintType, hidecategories = (Object, General), meta = (DontUseGenericSpawnObject = "true"))
class MULTITASK2_API USetDitheringTask : public UMultiThreadTask
{

	GENERATED_BODY()

public:

	virtual bool Start() override;

	virtual void TaskBody_Implementation() override;

	//virtual bool IsRunning() override;

public:
	FPixelData PixelData;
	int32 Scale;
};

class MULTITASK2_API FSetDitheringTaskTaskAction : public FSingleTaskActionBase
{
	EMultiTask2Branches& Branches;
	FPixelData& PixelData;
	bool bStarted;
public:
	FSetDitheringTaskTaskAction(UObject* InObject, EMultiTask2Branches& InBranches, const FLatentActionInfo& LatentInfo, const FPixelData& InPixelData, int32 Scale, const ETaskExecutionType& InExecutionType, UMultiTaskThreadPool* ThreadPool, UMultiTaskBase*& OutTask, FPixelData& OutPixelData)
		: FSingleTaskActionBase(InObject, LatentInfo, USetDitheringTask::StaticClass())
		, Branches(InBranches)
		, PixelData(OutPixelData)
		, bStarted(false)
	{
		OutTask = Task;

		USetDitheringTask* LocalTask = Cast<USetDitheringTask>(Task);
		if (LocalTask)
		{
			Branches = EMultiTask2Branches::OnStart;
			LocalTask->BodyFunction();
			LocalTask->PixelData = InPixelData;
			LocalTask->Scale = Scale;
			LocalTask->ExecutionType = InExecutionType;
			LocalTask->ThreadPool = ThreadPool;
			bStarted = Task->Start();
		}
		else {
			return;
		}
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		if (bStarted)
		{
			if (!IsCanceled())
			{
				if (!IsRunning())
				{
					USetDitheringTask* LocalTask = Cast<USetDitheringTask>(Task);
					if (LocalTask)
					{
						PixelData = MoveTemp(LocalTask->PixelData);
					}
					Branches = EMultiTask2Branches::OnCompleted;
					Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
				}
			}
			else {
				Branches = EMultiTask2Branches::OnCanceled;
				Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
			}
		}
		else {
			//If we reached this point it means the task was unable to start.
			Branches = EMultiTask2Branches::OnCompleted;
			Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
		}
	}
};