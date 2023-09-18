#include "BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(GetMesh());//Mesh 아래 항목으로 붙인다.
	CameraSpringArm->TargetArmLength = 600.0f;
	CameraSpringArm->bUsePawnControlRotation = true;//true: 마우스를 움직일 때 controller를 따라 SpringArm를 회전시킬 수 있다

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false; //Unequipped로 시작했을때 마우스를 돌릴때 화면만 돌아가지 않고 플레이어와 같이 돌아가도록 false 설정.
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABaseCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABaseCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABaseCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABaseCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABaseCharacter::LookUp);
}


void ABaseCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.0f)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);//캐릭터의 Yaw값을 에디터에서 -90도로 돌렸기 때문에 Controller->GetControlRotation().Yaw로 바뀐 값 사용.
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));//수정된 Yaw의 X방향의 FVector
		AddMovementInput(Direction, Value);//앞 방향으로 Value만큼 이동
	}

}

void ABaseCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));//수정된 Yaw의 Y방향의 FVector
		AddMovementInput(Direction, Value);//오른쪽 방향으로 Value만큼 이동
	}
}

void ABaseCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABaseCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABaseCharacter::Jump()
{
	Super::Jump();
}
