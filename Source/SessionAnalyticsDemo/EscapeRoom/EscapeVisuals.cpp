#include "EscapeRoom/EscapeVisuals.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace EscapeVisuals
{
	const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");

	UMaterialInterface* GetBaseMaterial()
	{
		static const TCHAR* BasePath = TEXT("/Game/EscapeRoom/Materials/M_EscapeBase.M_EscapeBase");
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, BasePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		return Material ? Material : LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	void SetColor(UMeshComponent* Mesh, const FLinearColor& Color, float Glow)
	{
		UMaterialInterface* Base = GetBaseMaterial();
		if (!Mesh || !Base)
		{
			return;
		}
		UMaterialInstanceDynamic* Instance = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
		if (!Instance || Instance->Parent != Base)
		{
			Instance = UMaterialInstanceDynamic::Create(Base, Mesh);
			Mesh->SetMaterial(0, Instance);
		}
		Instance->SetVectorParameterValue(TEXT("Color"), Color);
		Instance->SetScalarParameterValue(TEXT("Glow"), Glow);
	}

	void SetHintGlow(UMeshComponent* Mesh, float Amount)
	{
		if (!Mesh)
		{
			return;
		}
		for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
		{
			if (UMaterialInstanceDynamic* Instance = Mesh->CreateDynamicMaterialInstance(Index))
			{
				Instance->SetScalarParameterValue(TEXT("HintGlow"), Amount);
			}
		}
	}

	UStaticMeshComponent* CreatePart(UObject* Owner, FName Name, const TCHAR* ShapePath)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(ShapePath);
		if (Mesh.Succeeded())
		{
			Part->SetStaticMesh(Mesh.Object);
		}
		return Part;
	}
}
