// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "MultiThreadTask.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "UrlToDataTask.generated.h"

UENUM(BlueprintType)
enum class EURLRequestType : uint8
{
	GET,
	POST,
	PUT,
	DELETE,
	OPTIONS,
	HEAD,
	PATCH
};


UCLASS(HideDropdown, NotBlueprintable, NotBlueprintType, hidecategories = (Object, General), meta = (DontUseGenericSpawnObject = "true"))
class MULTITASK2_API UUrlToDataTask : public UMultiThreadTask
{

	GENERATED_BODY()

public:

	virtual bool Start() override;

	virtual void Cancel() override;

	virtual void TaskBody_Implementation() override;

	virtual bool IsRunning() override;

public:
	TArray<uint8> Data;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> URLRequest;
};

class MULTITASK2_API FUrlToDataTaskAction : public FSingleTaskActionBase
{
	EMultiTask2Branches& Branches;
	TArray<uint8>& Data;
	bool bStarted;
public:
	FUrlToDataTaskAction(UObject* InObject, EMultiTask2Branches& InBranches, const FLatentActionInfo& LatentInfo, const FString& URL, EURLRequestType RequestType, const TMap<FString, FString>& Headers, const TArray<uint8>& Content, float Timeout, const ETaskExecutionType& InExecutionType, UMultiTaskThreadPool* ThreadPool, UMultiTaskBase*& OutTask, TArray<uint8>& InData)
		: FSingleTaskActionBase(InObject, LatentInfo, UUrlToDataTask::StaticClass())
		, Branches(InBranches)
		, Data(InData)
		, bStarted(false)
	{
		Data.Empty();
		OutTask = Task;

		UUrlToDataTask* LocalTask = Cast<UUrlToDataTask>(Task);
		if (LocalTask)
		{
			Branches = EMultiTask2Branches::OnStart;
			LocalTask->BodyFunction();
			
			LocalTask->URLRequest = FHttpModule::Get().CreateRequest();
			
			if(RequestType == EURLRequestType::GET)
			{
				LocalTask->URLRequest->SetVerb("GET");        
			} else if(RequestType == EURLRequestType::POST)
			{
				LocalTask->URLRequest->SetVerb("POST");   
			} else if(RequestType == EURLRequestType::PUT)
			{
				LocalTask->URLRequest->SetVerb("PUT");   
			} else if(RequestType == EURLRequestType::DELETE)
			{
				LocalTask->URLRequest->SetVerb("DELETE");   
			} else if(RequestType == EURLRequestType::OPTIONS)
			{
				LocalTask->URLRequest->SetVerb("OPTIONS");   
			} else if(RequestType == EURLRequestType::HEAD)
			{
				LocalTask->URLRequest->SetVerb("HEAD");   
			} else if(RequestType == EURLRequestType::PATCH)
			{
				LocalTask->URLRequest->SetVerb("PATCH");   
			}

			for(auto& Header : Headers)
			{
				if(!Header.Key.IsEmpty() && !Header.Value.IsEmpty())
				{
					LocalTask->URLRequest->SetHeader(Header.Key, Header.Value);
				}
			}

			if(!Content.IsEmpty())
			{
				LocalTask->URLRequest->SetContent(Content);
			}
    
			LocalTask->URLRequest->SetURL(URL);

			if (!FMath::IsNearlyZero(Timeout))
			{
				LocalTask->URLRequest->SetTimeout(Timeout);
			}
			
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
					UUrlToDataTask* LocalTask = Cast<UUrlToDataTask>(Task);
					if (LocalTask)
					{
						Data = MoveTemp(LocalTask->Data);
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