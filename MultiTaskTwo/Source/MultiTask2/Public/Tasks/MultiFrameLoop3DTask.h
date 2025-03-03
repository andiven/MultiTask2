// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "MultiTaskBase.h"
#include "MultiFrameLoop3DTask.generated.h"

DECLARE_MULTICAST_DELEGATE(FLoop3DTaskDelegate);

UCLASS(Blueprintable, hidecategories = (Object), meta = (DontUseGenericSpawnObject = "True"))
class MULTITASK2_API UMultiFrameLoop3DTask : public UMultiTaskBase
{
	friend class FMultiFrameLoop3DTaskAction;
    GENERATED_BODY()

public:
    UMultiFrameLoop3DTask();
    ~UMultiFrameLoop3DTask();

    virtual bool Start() override;

    /**
    * Called on Game Thread on each index.
    */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, meta = (DisplayName = "Task Body"), Category = "Events")
        void TaskBody(int32 X, int32 Y, int32 Z);
    virtual void TaskBody_Implementation(int32 X, int32 Y, int32 Z);

    /**
    * Check whether the job is in progress.
    */
    virtual bool IsRunning() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true, ClampMin = "1", UIMin = "1"), Category = "General")
        int32 XSize = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true, ClampMin = "1", UIMin = "1"), Category = "General")
        int32 YSize = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true, ClampMin = "1", UIMin = "1"), Category = "General")
        int32 ZSize = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true, ClampMin = "1", UIMin = "1"), Category = "General")
        int32 IterationsPerTick = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true, ClampMin = "0.0", UIMin = "0.0"), Category = "General")
        float Delay = 0.0f;
    FLoop3DTaskDelegate TaskDelegate;
protected:
    virtual void Tick(float DeltaTime) override;

protected:
    int32 CurrentIndex = 0;
    bool bStarted = false;
    float TimeRemaining = 0.0f;
};


class MULTITASK2_API FMultiFrameLoop3DTaskAction : public FSingleTaskActionBase
{
private:
	int32& CurrentX;
	int32& CurrentY;
	int32& CurrentZ;
	int32 XSize;
	int32 YSize;
	int32 ZSize;
	EMultiTask2BranchesWithBody& Branches;
	bool bStarted;
public:

	FMultiFrameLoop3DTaskAction(UObject* InObject, EMultiTask2BranchesWithBody& InBranches, const FLatentActionInfo& LatentInfo, TSubclassOf<class UMultiFrameLoop3DTask> TaskClass, int32& InCurrentX, int32& InCurrentY, int32& InCurrentZ, const int32 InXSize, const int32 InYSize, const int32 InZSize, const int32 InIterationsPerTick, const float InDelay, UMultiFrameLoop3DTask*& OutTask)
		: FSingleTaskActionBase(InObject, LatentInfo, TaskClass)
		, CurrentX(InCurrentX)
		, CurrentY(InCurrentY)
		, CurrentZ(InCurrentZ)
		, XSize(InXSize)
		, YSize(InYSize)
		, ZSize(InZSize)
		, Branches(InBranches)
		, bStarted(false)
	{
		OutTask = Cast<UMultiFrameLoop3DTask>(Task);
		if (OutTask)
		{
			Branches = EMultiTask2BranchesWithBody::OnStart;
			OutTask->BodyFunction();
			OutTask->XSize = InXSize;
			OutTask->YSize = InYSize;
			OutTask->ZSize = InZSize;
			OutTask->IterationsPerTick = InIterationsPerTick;
			OutTask->Delay = InDelay;
			bStarted = Task->Start();
		}
		else {
			return;
		}

		if (bStarted)
		{
			OutTask->TaskDelegate.AddLambda([OutTask, &InBranches, &InCurrentX, &InCurrentY, &InCurrentZ]
			{
				InBranches = EMultiTask2BranchesWithBody::OnTaskBody;
				InCurrentX = OutTask->CurrentIndex % OutTask->XSize;
				InCurrentY = (OutTask->CurrentIndex / OutTask->XSize) % OutTask->YSize;
				InCurrentZ = OutTask->CurrentIndex / (OutTask->XSize * OutTask->YSize);
				OutTask->BodyFunction();
			});
		}
	}

	virtual ~FMultiFrameLoop3DTaskAction()
	{
		if (IsValid(Task) && !Task->HasAnyFlags(RF_BeginDestroyed) && !Task->IsUnreachable())
		{
			UMultiFrameLoop3DTask* LocalTask = Cast<UMultiFrameLoop3DTask>(Task);
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
			if (!IsCanceled())
			{
				if (!IsRunning())
				{
					Branches = EMultiTask2BranchesWithBody::OnCompleted;
					Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
				}
			}
			else {
				Branches = EMultiTask2BranchesWithBody::OnCanceled;
				Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
			}
		}
		else {
			//If we reached this point it means the task was unable to start.
			Branches = EMultiTask2BranchesWithBody::OnCompleted;
			Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
		}
	}
};