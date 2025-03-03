// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "MultiThreadTask.h"
#include "Math/Float16Color.h"
#include "Math/IntPoint.h"
#include "Math/Color.h"
#include "PixelFormat.h"
#include "RHI.h"
#ifndef ENGINE_MINOR_VERSION
#include "Runtime/Launch/Resources/Version.h"
#endif
#include "PixelReaderTask.generated.h"

USTRUCT(BlueprintType)
struct MULTITASK2_API FPixelData
{
	GENERATED_BODY()
	TArray64<uint8> Data;
	uint8 Type;
	int32 SizeX;
	int32 SizeY;
	bool sRGB = false;

	FPixelData()
		: Type(static_cast<uint8>(EPixelFormat::PF_B8G8R8A8))
		, SizeX(0)
		, SizeY(0)
	{}

	int32 GetWidth() const
	{
		return SizeX;
	}

	int32 GetHeight() const
	{
		return SizeY;
	}

	void SetWidth(const int32& InWidth)
	{
		SizeX = FMath::Clamp(InWidth, 0, InWidth);
		const EPixelFormat FormatType = static_cast<EPixelFormat>(Type);
		const int32 ExpectedDataSize = SizeX * SizeY * GPixelFormats[FormatType].BlockBytes;
		Data.SetNumZeroed(FMath::Clamp(ExpectedDataSize, 0, ExpectedDataSize));
	}

	void SetHeight(const int32& InHeight)
	{
		SizeY = FMath::Clamp(InHeight, 0, InHeight);
		const EPixelFormat FormatType = static_cast<EPixelFormat>(Type);
		const int32 ExpectedDataSize = SizeX * SizeY * GPixelFormats[FormatType].BlockBytes;
		Data.SetNumZeroed(FMath::Clamp(ExpectedDataSize, 0, ExpectedDataSize));
	}

	void SetSize(const int32& InWidth, const int32& InHeight)
	{
		SizeX = FMath::Clamp(InWidth, 0, InWidth);
		SizeY = FMath::Clamp(InHeight, 0, InHeight);
		const EPixelFormat FormatType = static_cast<EPixelFormat>(Type);
		const int32 ExpectedDataSize = SizeX * SizeY * GPixelFormats[FormatType].BlockBytes;
		Data.SetNumZeroed(FMath::Clamp(ExpectedDataSize, 0, ExpectedDataSize));
	}

	bool IsValid() const
	{
		if (SizeX > 0 && SizeY > 0)
		{
			const EPixelFormat FormatType = static_cast<EPixelFormat>(Type);
			const int32 ExpectedDataSize = SizeX * SizeY * GPixelFormats[FormatType].BlockBytes;
			return Data.Num() == ExpectedDataSize && ExpectedDataSize > 0;
		}
		return false;
	}

	bool SetPixel(const FIntPoint& Coordinates, const FColor& Pixel)
	{
		if (Coordinates.X >= 0 && Coordinates.Y >= 0 && Coordinates.X < SizeX && Coordinates.Y < SizeY && IsValid())
		{
			const EPixelFormat FormatType = static_cast<EPixelFormat>(Type);
			const int32 Size = GPixelFormats[FormatType].BlockBytes;
			const int32 CurrentPos = (SizeX * Coordinates.Y + Coordinates.X) * Size;
			if (CurrentPos + Size <= Data.Num())
			{
				uint8* DataPtr = Data.GetData();
				FLinearColor Color;
				FFloat16Color FloatColor;
				switch (FormatType)
				{
					case PF_FloatRGBA:
						FloatColor = FFloat16Color(FLinearColor(Pixel));
						FMemory::Memcpy(&DataPtr[CurrentPos], &FloatColor, Size);
						break;
					case PF_A32B32G32R32F:
						Color = FLinearColor(Pixel);
						FMemory::Memcpy(&DataPtr[CurrentPos], &Pixel, Size);
						break;
					default:
						FMemory::Memcpy(&DataPtr[CurrentPos], &Pixel, Size);
						break;

				}
				return true;
			}
		}
		return false;
	}

	bool GetPixel(const FIntPoint& Coordinates, FColor& Pixel) const
	{
		if (Coordinates.X >= 0 && Coordinates.Y >= 0 && Coordinates.X < SizeX && Coordinates.Y < SizeY && IsValid())
		{
			const EPixelFormat FormatType = static_cast<EPixelFormat>(Type);
			const int32 Size = GPixelFormats[FormatType].BlockBytes;
			const int32 CurrentPos = (SizeX * Coordinates.Y + Coordinates.X) * Size;
			if (CurrentPos + Size <= Data.Num())
			{
				const uint8* DataPtr = Data.GetData();
				switch (FormatType)
				{
					case PF_FloatRGBA:
					{
						FFloat16Color Output;
						FMemory::Memcpy(&Output, &DataPtr[CurrentPos], Size);
						Pixel = FLinearColor(Output).ToFColor(sRGB);
						break;
					}
					case PF_A32B32G32R32F:
					{
						FLinearColor Output;
						FMemory::Memcpy(&Output, &DataPtr[CurrentPos], Size);
						Pixel = Output.ToFColor(sRGB);
						break;
					}
					default:
					{
						FColor Output;
						FMemory::Memcpy(&Output, &DataPtr[CurrentPos], Size);
						Pixel = Output;
						break;
					}
				}
				return true;
			}
		}

		return false;
	}

	friend FArchive& operator<<(FArchive& Ar, FPixelData& PixelData)
	{
		Ar << PixelData.Data;
		Ar << PixelData.Type;
		Ar << PixelData.SizeX;
		Ar << PixelData.SizeY;
		Ar << PixelData.sRGB;
		return Ar;
	}
};

class UTexture;

UCLASS(HideDropdown, NotBlueprintable, NotBlueprintType, hidecategories = (Object, General), meta = (DontUseGenericSpawnObject = "true"))
class MULTITASK2_API UPixelReaderTask : public UMultiThreadTask
{

	GENERATED_BODY()

public:

	virtual bool Start() override;

	virtual void TaskBody_Implementation() override;

	virtual bool IsRunning() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
		UTexture* TextureObj;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
		FPixelData PixelData;
protected:
	bool bCompleted = false;
};

class MULTITASK2_API FPixelReaderTaskAction : public FSingleTaskActionBase
{
	EMultiTask2BranchesNoCancel& Branches;
	FPixelData& PixelData;
	bool bStarted;
public:
	FPixelReaderTaskAction(UObject* InObject, EMultiTask2BranchesNoCancel& InBranches, const FLatentActionInfo& LatentInfo, UTexture* InTextureObj, FPixelData& InPixelData)
		: FSingleTaskActionBase(InObject, LatentInfo, UPixelReaderTask::StaticClass())
		, Branches(InBranches)
		, PixelData(InPixelData)
		, bStarted(false)
	{
		UPixelReaderTask* LocalTask = Cast<UPixelReaderTask>(Task);
		if (LocalTask)
		{
			Branches = EMultiTask2BranchesNoCancel::OnStart;
			LocalTask->BodyFunction();
			LocalTask->TextureObj = InTextureObj;
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
			if (!IsRunning())
			{
				UPixelReaderTask* LocalTask = Cast<UPixelReaderTask>(Task);
				if (LocalTask)
				{
					PixelData = MoveTemp(LocalTask->PixelData);
				}
				Branches = EMultiTask2BranchesNoCancel::OnCompleted;
				Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
			}
		}
		else {
			//If we reached this point it means the task was unable to start.
			Branches = EMultiTask2BranchesNoCancel::OnCompleted;
			Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
		}
	}
};