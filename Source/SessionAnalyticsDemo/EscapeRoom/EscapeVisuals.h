#pragma once

#include "CoreMinimal.h"

class UMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/** Gray-box helpers shared by the escape-room actors. */
namespace EscapeVisuals
{
	/** /Game/EscapeRoom/Materials/M_EscapeBase (params: Color, Glow, HintGlow). Falls back to the engine shape material. */
	UMaterialInterface* GetBaseMaterial();

	/** Applies the base material as a dynamic instance and sets its color and constant glow. */
	void SetColor(UMeshComponent* Mesh, const FLinearColor& Color, float Glow = 0.f);

	/** Sets the hint glow on every material slot of the mesh (0 = off). */
	void SetHintGlow(UMeshComponent* Mesh, float Amount);

	/** Creates a static mesh sub-object in a constructor using one of the engine basic shapes. */
	UStaticMeshComponent* CreatePart(UObject* Owner, FName Name, const TCHAR* ShapePath);

	extern const TCHAR* Cube;
	extern const TCHAR* Cylinder;
	extern const TCHAR* Sphere;
}
