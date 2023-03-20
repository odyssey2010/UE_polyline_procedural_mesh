// MIT License
// Copyright (c) 2023 devx.3dcodekits.com
// All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this softwareand associated documentation
// files(the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions :
//
// The above copyright noticeand this permission notice shall be
// included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "MyPolylineActor.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AMyPolylineActor::AMyPolylineActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
	ProceduralMesh->SetupAttachment(RootComponent);

	static FString Path = TEXT("/Game/M_PolylineMaterial");
	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialLoader(*Path);

	if (MaterialLoader.Succeeded())
	{
		Material = MaterialLoader.Object;

		ProceduralMesh->SetMaterial(0, Material);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load material at path: %s"), *Path);
	}
}

void AMyPolylineActor::BeginPlay()
{
	Super::BeginPlay();
	
	BuildMesh();
}

void AMyPolylineActor::PostLoad()
{
	Super::PostLoad();

	BuildMesh();
}

#define DEBUGMESSAGE(x, ...) if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Red, FString::Printf(TEXT(x), __VA_ARGS__));}

void AMyPolylineActor::BuildPolyline()
{
	TArray<FVector> Vertices;
	TArray<FLinearColor> Colors;
	TArray<int32> Triangles;

	FVector Point, PrevPoint, NextPoint;
	FVector PrevDir, NextDir, PrevOutDir, NextOutDir;
	FVector OutDir, UpDir = FVector::ZAxisVector; // Outward, Upward Direction
	FVector V0, V1, PrevV0, PrevV1;
	int32 I0, I1, PrevI0 = 0, PrevI1 = 0;

	const float HalfThickness = Thickness * 0.5f;
	float OutDirScale = 1.0f;

	for (int32 i = 0, NumPoly = Points.Num(); i < NumPoly; ++i)
	{
		Point = Points[i];
		NextPoint = Points[(i < NumPoly - 1) ? (i + 1) : 0];

		if (i == 0)
		{
			PrevDir = (NextPoint - Point).GetSafeNormal();
			OutDir = UpDir.Cross(PrevDir);
			OutDirScale = 1.0f;
		}
		else if (i == NumPoly - 1)
		{
			PrevDir = (Point - PrevPoint).GetSafeNormal();
			OutDir = UpDir.Cross(PrevDir);
			OutDirScale = 1.0f;
		}
		else
		{
			PrevDir = (Point - PrevPoint).GetSafeNormal();
			PrevOutDir = UpDir.Cross(PrevDir);

			NextDir = (NextPoint - Point).GetSafeNormal();
			NextOutDir = UpDir.Cross(NextDir);

			OutDir = (PrevOutDir + NextOutDir).GetSafeNormal();	// Average normal

			float CosTheta = NextDir.Dot(-PrevDir);
			float Angle = FMath::Acos(CosTheta);

			float SinValue = FMath::Sin(Angle * 0.5f);
			OutDirScale = FMath::IsNearlyZero(SinValue, 0.01f) ? 1000.0f : (1.0f / SinValue);
		}

		V0 = Point + OutDir * HalfThickness * OutDirScale;
		V1 = Point - OutDir * HalfThickness * OutDirScale;

		I0 = Vertices.Add(V0);
		I1 = Vertices.Add(V1);

		Colors.Add(Color);
		Colors.Add(Color);

		if (i > 0)	// Add rectangle of a edge segment
		{
			Triangles.Append({ I0, I1, PrevI0 });
			Triangles.Append({ PrevI1, PrevI0, I1 });
		}

		PrevV0 = V0;
		PrevV1 = V1;

		PrevI0 = I0;
		PrevI1 = I1;

		PrevPoint = Point;
	}

	ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, {}, {}, Colors, {}, false);
}

// Called every frame
void AMyPolylineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float Roll = RotationSpeed.X * DeltaTime;
	float Pitch = RotationSpeed.Y * DeltaTime;
	float Yaw = RotationSpeed.Z * DeltaTime;

	FQuat Curr = GetActorRotation().Quaternion();
	FQuat Add = FRotator(Pitch, Yaw, Roll).Quaternion();
	FQuat Rotation = Curr * Add;

	SetActorRotation(Rotation);
}

void AMyPolylineActor::BuildMesh()
{
	BuildPolyline();
}

void AMyPolylineActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property) 
	{
		BuildMesh();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
