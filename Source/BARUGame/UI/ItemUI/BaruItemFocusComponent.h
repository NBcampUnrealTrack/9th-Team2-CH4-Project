#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaruItemFocusComponent.generated.h"

class UBaruItemFocusWidget;

// PlayerController BP에 하나만 추가. 로컬 화면에서만 표시/추적합니다.
UCLASS(ClassGroup = (BARU), meta = (BlueprintSpawnableComponent))
class BARUGAME_API UBaruItemFocusComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBaruItemFocusComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Item Focus")
    bool bEnableItemFocusUI = true;

    // 현재 Character의 InteractionTraceDistance와 동일하게 설정합니다(첨부 소스 기본 300cm).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Focus",
        meta = (ClampMin = "1.0", Units = "cm"))
    float FocusTraceDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Focus")
    FVector2D CardAnchor = FVector2D(0.5f, 0.78f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Focus",
        meta = (ClampMin = "320.0", ClampMax = "800.0"))
    float CardWidth = 440.0f;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UBaruItemFocusWidget> FocusWidget;
};
