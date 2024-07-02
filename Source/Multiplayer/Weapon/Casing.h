#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

class USoundCue;

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
	TObjectPtr<UStaticMeshComponent> CasingMesh; //�Ѿ�ź�� �Ž�

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ShellSound; //�Ѿ�ź�ǰ� �ε������� ���� �Ҹ�

	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse; //�Ѿ�ź�ǰ� ������ �ӵ���

};
