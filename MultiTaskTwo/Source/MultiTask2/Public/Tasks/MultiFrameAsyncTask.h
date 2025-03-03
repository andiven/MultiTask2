// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "MultiTaskBase.h"
#include "MultiFrameAsyncTask.generated.h"

DECLARE_MULTICAST_DELEGATE(FAsyncTaskDelegate);

UCLASS(Blueprintable, hidecategories = (Object), meta = (DontUseGenericSpawnObject = "True"))
class MULTITASK2_API UMultiFrameAsyncTask : public UMultiTaskBase
{
	GENERATED_BODY()

public:
	UMultiFrameAsyncTask();
	~UMultiFrameAsyncTask();

	virtual bool Start() override;

	/**
	* Called on Game Thread on each index.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, meta = (DisplayName = "Task Body"), Category = "Events")
		void TaskBody();
	virtual void TaskBody_Implementation();

	/**
	* Check whether the job is in progress.
	*/
	virtual bool IsRunning() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true, ClampMin = "1", UIMin = "1"), Category = "General")
		int32 IterationsPerTick = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true, ClampMin = "0.0", UIMin = "0.0"), Category = "General")
		float Delay = 0.0f;
	FAsyncTaskDelegate TaskDelegate;
protected:
	virtual void Tick(float DeltaTime) override;

protected:
	bool bStarted = false;
	float TimeRemaining = 0.0f;
};


class MULTITASK2_API FMultiFrameAsyncTaskAction : public FSingleTaskActionBase
{
private:
	EMultiTask2BranchesNoCompleteWithBody& Branches;
	bool bStarted;
public:

	FMultiFrameAsyncTaskAction(UObject* InObject, EMultiTask2BranchesNoCompleteWithBody& InBranches, const FLatentActionInfo& LatentInfo, TSubclassOf<class UMultiFrameAsyncTask> TaskClass, const int32 InIterationsPerTick, const float InDelay, UMultiFrameAsyncTask*& OutTask)
		: FSingleTaskActionBase(InObject, LatentInfo, TaskClass)
		, Branches(InBranches)
		, bStarted(false)
	{

		OutTask = Cast<UMultiFrameAsyncTask>(Task);

		if (OutTask)
		{
			Branches = EMultiTask2BranchesNoCompleteWithBody::OnStart;
			OutTask->BodyFunction();
			OutTask->IterationsPerTick = InIterationsPerTick;
			OutTask->Delay = InDelay;
			bStarted = Task->Start();
		}
		else {
			return;
		}

		if (bStarted)
		{
			OutTask->TaskDelegate.AddLambda([OutTask, &InBranches]
			{
				InBranches = EMultiTask2BranchesNoCompleteWithBody::OnTaskBody;
				OutTask->BodyFunction();
			});
		}
	}

	virtual ~FMultiFrameAsyncTaskAction()
	{
		if (IsValid(Task) && !Task->HasAnyFlags(RF_BeginDestroyed) && !Task->IsUnreachable())
		{
			UMultiFrameAsyncTask* LocalTask = Cast<UMultiFrameAsyncTask>(Task);
			if (LocalTask)
			{
				LocalTask->TaskDelegate.RemoveAll(this);
			}
		}
	}


	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		if (bStarted)
		{
			if (IsCanceled())
			{
				Branches = EMultiTask2BranchesNoCompleteWithBody::OnCanceled;
				Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
			}
		}
		else {
			//If we reached this point it means the task was unable to start.
			Branches = EMultiTask2BranchesNoCompleteWithBody::OnCanceled;
			Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
		}
	}
};