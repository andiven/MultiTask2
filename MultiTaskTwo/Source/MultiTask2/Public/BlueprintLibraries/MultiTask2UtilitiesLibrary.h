// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Delegates/IDelegateInstance.h"
#include "MultiTask2UtilitiesLibrary.generated.h"

class UMultiTaskBase;
class UHierarchicalInstancedStaticMeshComponent;
class UGameInstance;

UENUM(BlueprintType)
enum class EMultiTask2ScaleType : uint8
{
    Uniform,
    Free,
    LockXY,
    LockXZ,
    LockYZ
};

UCLASS()
class MULTITASK2_API UMultiTask2UtilitiesLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
        UMultiTask2UtilitiesLibrary();
    ~UMultiTask2UtilitiesLibrary();
public:
    /**
    * Set the Maximum Loop Iteration value. 
    */
    UFUNCTION(BlueprintCallable, Category = "Multi Task 2|Utilities")
        static void SetMaximumLoopIterations(int32 MaximumLoopIterations);

    /**
    * Resets the runaway loop detection to prevent breaking loops.
    */
    UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe), Category = "Multi Task 2|Utilities")
        static void ResetRunaway();

    /**
    * Get World.
    */
    UFUNCTION(BlueprintPure, Category = "Multi Task 2|Utilities")
        static UObject* GetContextWorld(UObject* WorldContextObject);

    /**
    * Empty all HISM instance bodies
    * Always call this before using Set Collision Enabled
    * @param HISM			HISM Component.
    */  
    UFUNCTION(BlueprintCallable, Category = "Multi Task 2|Utilities")
        static void ClearInstancePhysicsBodies(UHierarchicalInstancedStaticMeshComponent* HISM);

    /**
	* Update Physics Body for HISM instance
	* @param HISM			HISM Component.
	* @param InstanceIndex	Instance to update physics body for.
	* @param bTeleport		Whether or not the instance should be moved normaly, or teleported (moved instantly, ignoring velocity).
    * @return True if success
	*/
    UFUNCTION(BlueprintCallable, Category = "Multi Task 2|Utilities")
       static bool UpdateInstancePhysicsBody(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, bool bTeleport);

    /**
    * Attempts to Cancel the Task. 
    */
    UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe, DisplayName = "Cancel"), Category = "Multi Task 2|Task")
        static void Cancel(UMultiTaskBase* Task);

    /**
    * Check whether the task is in progress.
    */
    UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe, DisplayName = "IsRunning", CompactNodeTitle = "IsRunning"), Category = "Multi Task 2|Task")
        static bool IsRunning(UMultiTaskBase* Task);

    /**
    * Check whether the task is canceled.
    */
    UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe, DisplayName = "IsCanceled", CompactNodeTitle = "IsCanceled"), Category = "Multi Task 2|Task")
        static bool IsCanceled(UMultiTaskBase* Task);

    UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe), Category = "Multi Task 2|Utilities")
        static int32 MixThreeIntegers(int32 Integer1, int32 Integer2, int32 Integer3);

    UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe), Category = "Multi Task 2|Utilities")
        static void GetRandomPointInTriangle(const FVector& A, const FVector& B, const FVector& C, const FRandomStream& RandomStream, FVector& OutPoint);

    UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe), Category = "Multi Task 2|Utilities")
        static float GetTriangleAreaSize(const FVector& A, const FVector& B, const FVector& C);

    UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe), Category = "Multi Task 2|Utilities")
        static void GetRandomScale(const FVector& Min, const FVector& Max, EMultiTask2ScaleType Type, const FRandomStream& RandomStream, FVector& Scale);

    UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe), Category = "Multi Task 2|Utilities")
        static bool SegmentTriangleIntersection(const FVector& StartPoint, const FVector& EndPoint, const FVector& A, const FVector& B, const FVector& C, FVector& OutIntersectPoint, FVector& OutTriangleNormal);

    /**
    * Update Navigation Data
    * @param    Component			The component to update navigation data for.
    */
    UFUNCTION(BlueprintCallable, Category = "Multi Task 2|Marching Cubes|Utilities")
        static void UpdateNavigationData(UActorComponent* Component);

    /**
    * Bind a Function to OnWindowCloseRequested event.
    * This allows users to prevent and add some pre-requisites before the game receives a sudden death request(Alt+F4 etc)
    * Selected function should have a bool return value.
    */
    UFUNCTION(BlueprintCallable, Category = "Multi Task 2|Utilities", Meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
        static void BindOnWindowCloseRequested(UObject* WorldContextObject, UGameInstance* Instance, FName FunctionName);

    /**
    * Returns Editor Camera transform
    * This function intended to be used in simulation mode.
    * @param Transform Camera Transform. FTransform::Identity if failed
    * @return True if success
    */
    UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe), Category = "Multi Task 2|Utilities")
        static bool GetEditorCameraTransform(FTransform& Transform);

    /**
    * Add an object to Root Set preventing it being collected by GC.
    * When the object not needed remove it from the Root Set.
    */
    UFUNCTION(BlueprintCallable, Category = "Multi Task 2|Utilities")
        static void AddToRoot(UObject* Object);

    /**
    * Remove an object from the Root Set.
    */
    UFUNCTION(BlueprintCallable, Category = "Multi Task 2|Utilities")
        static void RemoveFromRoot(UObject* Object);

private:
    static void OnEndPIE(const bool bIsSimulating);
    static void OnPreExit();
public:
#if WITH_EDITOR
    static FDelegateHandle EndPIEHandle;
#else
    static FDelegateHandle PreExitHandle;
#endif
    static TArray<UObject*> RootObjects;
    static int32 TaskIndex;
    static int32 MutexIndex;
    static int32 ThreadPoolIndex;
};
