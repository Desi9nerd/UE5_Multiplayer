#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class MULTIPLAYER_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh; //ÃÑ¾ËÅºÇÇ ¸Å½¬

	UPROPERTY(EditAnywhere)
	class USoundCue* ShellSound; //ÃÑ¾ËÅºÇÇ°¡ ºÎµúÇûÀ»¶§ ³ª´Â ¼Ò¸®

	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse; //ÃÑ¾ËÅºÇÇ°¡ ³ª°¥¶§ ¼Óµµ°ª

};
