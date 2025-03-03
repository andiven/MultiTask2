// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#include "PixelReaderTask.h"

#include "Engine/Texture.h"
#include "RHIResources.h"
#include "TextureResource.h"
#include "RHI.h"

#include "RHICommandList.h"
#include "HAL/UnrealMemory.h"
#include "Math/IntRect.h"
#include "RenderingThread.h"
#include "ImagePixelData.h"
#include "Engine/TextureRenderTarget2D.h"
#ifndef ENGINE_MINOR_VERSION
#include "Runtime/Launch/Resources/Version.h"

#include "Rendering/Texture2DResource.h"

#endif
bool UPixelReaderTask::Start()
{

    bCanceled = false;

    UPixelReaderTask* Worker = this;

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
    Tasks[0] = Async(EAsyncExecution::TaskGraphMainThread, TUniqueFunction<void()>(BodyFunc), TUniqueFunction<void()>(OnCompleteFunc));

    return true;
}

void UPixelReaderTask::TaskBody_Implementation()
{
	UPixelReaderTask* Worker = this;

	ENQUEUE_RENDER_COMMAND(PixelResolve)([Worker](FRHICommandListImmediate& RHICmdList)
	{
		FTextureResource* TextureResource = Worker->TextureObj->GetResource();

		FRHITexture2D* Texture2D = TextureResource->GetTexture2DRHI();
		
		if (!Texture2D)
		{
			Worker->bCompleted = true;
			return;
		}

		const EPixelFormat Format = Texture2D->GetFormat();

		switch (Format)
		{
		case PF_FloatRGBA:
			Worker->PixelData.Type = static_cast<uint8>(Format);
			break;
		case PF_A32B32G32R32F:
			Worker->PixelData.Type = static_cast<uint8>(Format);
			break;
		case PF_R8G8B8A8:
			Worker->PixelData.Type = static_cast<uint8>(Format);
			break;
		case PF_B8G8R8A8:
			Worker->PixelData.Type = static_cast<uint8>(Format);
			break;
		default:
			Worker->bCompleted = true;
			return;
		}

		const int32 LocalSizeX = Texture2D->GetSizeX();
		const int32 LocalSizeY = Texture2D->GetSizeY();

		if (LocalSizeX * LocalSizeY <= 0)
		{
			Worker->bCompleted = true;
			return;
		}

		Worker->PixelData.SizeX = LocalSizeX;
		Worker->PixelData.SizeY = LocalSizeY;

		const FIntRect SourceRect(0, 0, Texture2D->GetSizeX(), Texture2D->GetSizeY());
		switch (Texture2D->GetFormat())
		{
		case PF_FloatRGBA:
		{
			TArray<FFloat16Color> RawPixels;
			RawPixels.SetNum(SourceRect.Width() * SourceRect.Height());

			RHICmdList.ReadSurfaceFloatData(Texture2D, SourceRect, RawPixels, (ECubeFace)0, 0, 0);
				
			Worker->PixelData.Data.SetNumUninitialized(RawPixels.Num() * sizeof(FFloat16Color));
			FMemory::Memcpy(Worker->PixelData.Data.GetData(), RawPixels.GetData(), RawPixels.Num() * sizeof(FFloat16Color));
			break;
		}

		case PF_A32B32G32R32F:
		{
			FReadSurfaceDataFlags ReadDataFlags(RCM_MinMax);
			ReadDataFlags.SetLinearToGamma(false);


			TArray<FLinearColor> RawPixels;
			RawPixels.SetNum(SourceRect.Width() * SourceRect.Height());
			RHICmdList.ReadSurfaceData(Texture2D, SourceRect, RawPixels, ReadDataFlags);

			Worker->PixelData.Data.SetNumUninitialized(RawPixels.Num() * sizeof(FLinearColor));
			FMemory::Memcpy(Worker->PixelData.Data.GetData(), RawPixels.GetData(), RawPixels.Num() * sizeof(FLinearColor));
			break;
		}

		case PF_R8G8B8A8:
		case PF_B8G8R8A8:
		{
			FReadSurfaceDataFlags ReadDataFlags;
			ReadDataFlags.SetLinearToGamma(false);

			TArray<FColor> RawPixels;
			RawPixels.SetNum(SourceRect.Width() * SourceRect.Height());
			RHICmdList.ReadSurfaceData(Texture2D, SourceRect, RawPixels, ReadDataFlags);
				
			Worker->PixelData.Data.SetNumUninitialized(RawPixels.Num() * sizeof(FColor));
			FMemory::Memcpy(Worker->PixelData.Data.GetData(), RawPixels.GetData(), RawPixels.Num() * sizeof(FColor));
			break;
		}

		default:
			break;
		}
		
		Worker->bCompleted = true;
	});

	FlushRenderingCommands();
}

bool UPixelReaderTask::IsRunning()
{
	return bCompleted ? false : true;
}