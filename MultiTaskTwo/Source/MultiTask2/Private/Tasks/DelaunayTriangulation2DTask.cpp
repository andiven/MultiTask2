// Copyright 2017-2022 S.C. Pug Life Studio S.R.L. All Rights Reserved.
#include "DelaunayTriangulation2DTask.h"
#include "HAL/UnrealMemory.h"
#include "MultiTaskThreadPool.h"
#ifndef ENGINE_MINOR_VERSION
#include "Runtime/Launch/Resources/Version.h"
#endif

struct FMultiTask2Delaunay2DEdge
{
	const int32 v;
	const int32 w;
	bool bIsInvalid;

	FMultiTask2Delaunay2DEdge(const int32& v1, const int32& v2)
		: v(v1)
		, w(v2)
		, bIsInvalid(false) {}

};

static bool IsNearlyEqual(const double& x, const double& y, const double& ErrorTolerance = DOUBLE_KINDA_SMALL_NUMBER)
{
	return FMath::IsNearlyEqual(x, y, ErrorTolerance);
}

static bool IsNearlyEqual(const FVector2D& v1, const FVector2D& v2, const double& ErrorTolerance = DOUBLE_KINDA_SMALL_NUMBER)
{
	return IsNearlyEqual(v1.X, v2.X, ErrorTolerance) && IsNearlyEqual(v1.Y, v2.Y, ErrorTolerance);
}

static bool IsNearlyEqual(const TArray<FVector2D>& Vertices, const FMultiTask2Delaunay2DEdge& e1, const FMultiTask2Delaunay2DEdge& e2, const double& ErrorTolerance = DOUBLE_KINDA_SMALL_NUMBER)
{
	return	(IsNearlyEqual(Vertices[e1.v], Vertices[e2.v], ErrorTolerance) && IsNearlyEqual(Vertices[e1.w], Vertices[e2.w], ErrorTolerance)) || (IsNearlyEqual(Vertices[e1.v], Vertices[e2.w], ErrorTolerance) && IsNearlyEqual(Vertices[e1.w], Vertices[e2.v], ErrorTolerance));
}

bool FMultiTask2Delaunay2DTriangle::ContainsVertex(const TArray<FVector2D>& Vertices, const int32& v) const
{
	return IsNearlyEqual(Vertices[a], Vertices[v]) || IsNearlyEqual(Vertices[b], Vertices[v]) || IsNearlyEqual(Vertices[c], Vertices[v]);
}

bool FMultiTask2Delaunay2DTriangle::CircumCircleContains(const TArray<FVector2D>& Vertices, const int32& v) const
{
	const FVector2D& VectorA = Vertices[a];
	const FVector2D& VectorB = Vertices[b];
	const FVector2D& VectorC = Vertices[c];
	const FVector2D& VectorV = Vertices[v];

	const double ab = VectorA.SizeSquared();
	const double cd = VectorB.SizeSquared();
	const double ef = VectorC.SizeSquared();

	const double circum_x = (ab * (VectorC.Y - VectorB.Y) + cd * (VectorA.Y - VectorC.Y) + ef * (VectorB.Y - VectorA.Y)) / (VectorA.X * (VectorC.Y - VectorB.Y) + VectorB.X * (VectorA.Y - VectorC.Y) + VectorC.X * (VectorB.Y - VectorA.Y));
	const double circum_y = (ab * (VectorC.X - VectorB.X) + cd * (VectorA.X - VectorC.X) + ef * (VectorB.X - VectorA.X)) / (VectorA.Y * (VectorC.X - VectorB.X) + VectorB.Y * (VectorA.X - VectorC.X) + VectorC.Y * (VectorB.X - VectorA.X));

	const FVector2D circum(circum_x / 2, circum_y / 2);
	const double circum_radius = FVector2D::DistSquared(VectorA, circum);
	const double dist = FVector2D::DistSquared(VectorV, circum);
	return dist <= circum_radius;
}

bool UDelaunayTriangulation2DTask::Start()
{
    bCanceled = false;

	if (Vertices.Num() < 3)
	{
		return false;
	}

    UDelaunayTriangulation2DTask* Worker = this;

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

void UDelaunayTriangulation2DTask::TaskBody_Implementation()
{
	Triangles.Empty();
	if (Vertices.Num() >= 3)
	{
		double minX = Vertices[0].X;
		double minY = Vertices[0].Y;
		double maxX = minX;
		double maxY = minY;

		for (int32 i = 0; i < Vertices.Num(); ++i)
		{
			if (IsCanceled())
			{
				return;
			}
			if (Vertices[i].X < minX) minX = Vertices[i].X;
			if (Vertices[i].Y < minY) minY = Vertices[i].Y;
			if (Vertices[i].X > maxX) maxX = Vertices[i].X;
			if (Vertices[i].Y > maxY) maxY = Vertices[i].Y;
		}

		const double dx = maxX - minX;
		const double dy = maxY - minY;
		const double deltaMax = FMath::Max(dx, dy);
		const double midx = (minX + maxX) / 2;
		const double midy = (minY + maxY) / 2;

		const FVector2D p1(midx - 20 * deltaMax, midy - deltaMax);
		const FVector2D p2(midx, midy + 20 * deltaMax);
		const FVector2D p3(midx + 20 * deltaMax, midy - deltaMax);

		Vertices.Add(p1);
		Vertices.Add(p2);
		Vertices.Add(p3);

		const int expandVtxIdx = Vertices.Num() - 3;

		Triangles.Add(FMultiTask2Delaunay2DTriangle(expandVtxIdx, expandVtxIdx + 1, expandVtxIdx + 2));

		for (int32 VertIdx = 0; VertIdx < Vertices.Num(); VertIdx++)
		{
			const FVector2D& Point = Vertices[VertIdx];

			if (IsCanceled())
			{
				return;
			}
			TArray<FMultiTask2Delaunay2DEdge> Polygon;

			if (Triangles.Num() > 0)
			{
				for (int32 TriIndex = Triangles.Num() - 1; TriIndex >= 0; --TriIndex)
				{
					if (IsCanceled())
					{
						return;
					}
					FMultiTask2Delaunay2DTriangle& Triangle = Triangles[TriIndex];
					if (Triangle.CircumCircleContains(Vertices, VertIdx))
					{
						Polygon.Add(FMultiTask2Delaunay2DEdge(Triangle.a, Triangle.b));
						Polygon.Add(FMultiTask2Delaunay2DEdge(Triangle.b, Triangle.c));
						Polygon.Add(FMultiTask2Delaunay2DEdge(Triangle.c, Triangle.a));
						Triangles.RemoveAt(TriIndex);
					}
				}
			}

			if (Polygon.Num() > 0)
			{
				for (int32 EdgeIndex = 0; EdgeIndex < Polygon.Num(); EdgeIndex++)
				{
					for (int32 NextEdgeIndex = EdgeIndex + 1; NextEdgeIndex < Polygon.Num(); NextEdgeIndex++)
					{
						if (IsCanceled())
						{
							return;
						}
						if (!Polygon[EdgeIndex].bIsInvalid && !Polygon[NextEdgeIndex].bIsInvalid)
						{
							if (IsNearlyEqual(Vertices, Polygon[EdgeIndex], Polygon[NextEdgeIndex]))
							{
								Polygon[EdgeIndex].bIsInvalid = true;
								Polygon[NextEdgeIndex].bIsInvalid = true;
							}
						}
					}
				}
			}

			if (Polygon.Num() > 0)
			{
				for (int32 EdgeIndex = Polygon.Num() - 1; EdgeIndex >= 0; --EdgeIndex)
				{
					if (IsCanceled())
					{
						return;
					}
					if (!Polygon[EdgeIndex].bIsInvalid)
					{
						Triangles.Add(FMultiTask2Delaunay2DTriangle(Polygon[EdgeIndex].v, Polygon[EdgeIndex].w, VertIdx));
					}
				}
			}
		}

		if (Triangles.Num() > 0)
		{
			for (int32 TriIndex = Triangles.Num() - 1; TriIndex >= 0; --TriIndex)
			{
				if (IsCanceled())
				{
					return;
				}
				FMultiTask2Delaunay2DTriangle& Triangle = Triangles[TriIndex];
				if (Triangle.ContainsVertex(Vertices, expandVtxIdx) || Triangle.ContainsVertex(Vertices, expandVtxIdx + 1) || Triangle.ContainsVertex(Vertices, expandVtxIdx + 2))
				{
					Triangles.RemoveAt(TriIndex);
					continue;
				}
			}
		}
	}
}