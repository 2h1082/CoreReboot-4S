#include "Game/System/EnvironmentManager.h"
#include "Engine/DataAsset.h"
#include "Game/System/WorldTimeManager.h"
#include "Game/System/SeasonType.h"
#include "Game/System/SpawnZoneManager.h"
#include "Game/System/SeasonManager.h"
#include "Game/System/EnvironmentalModifierVolume.h"

AEnvironmentManager::AEnvironmentManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnvironmentManager::BeginPlay()
{
    // Load Weather if there is Save

    //For NewGame, Season Starts with Bountiful Season
    UE_LOG(LogTemp, Warning, TEXT("BeginPlay time: %.6f"), FPlatformTime::Seconds());


    TimeManager = GetWorld()->GetSubsystem<UWorldTimeManager>();
    if (TimeManager)
    {
        TimeManager->SetWorldTimeMultiplier(WorldTimeMultiplier);
        TimeManager->StartWorldTime();
    }

    Super::BeginPlay();
}

void AEnvironmentManager::SetDayNightByTime(float DawnTime, float DuskTime)
{

    DawnTime = FMath::Clamp(DawnTime, 0.0f, 2400.0f);
    DuskTime = FMath::Clamp(DuskTime, 0.0f, 2400.0f);

    SetDawnDuskTime(DawnTime, DuskTime);
}

void AEnvironmentManager::SetWeatherByName(const FString& WeatherName, float TransitionTime)
{
    FString AssetPath = FString::Printf(TEXT("/Game/UltraDynamicSky/Blueprints/Weather_Effects/Weather_Presets/%s.%s"), *WeatherName, *WeatherName);

    UObject* LoadedObject = StaticLoadObject(UDataAsset::StaticClass(), nullptr, *AssetPath);

    if (UDataAsset* WeatherAsset = Cast<UDataAsset>(LoadedObject))
    {
        UE_LOG(LogTemp, Log, TEXT("Weather preset loaded: %s"), *WeatherName);
        ApplyWeather(WeatherAsset, TransitionTime);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load weather preset: %s"), *AssetPath);
    }
}

void AEnvironmentManager::SetWeatherBySeason(ESeasonType Season, float TransitionTime)
{
    UDataAsset* SelectedPreset = nullptr;

    switch (Season)
    {
    case ESeasonType::BountifulSeason:
        SelectedPreset = BountifulSeasonPreset;
        break;
    case ESeasonType::FrostSeason:
        SelectedPreset = FrostSeasonPreset;
        break;
    case ESeasonType::RainySeason:
        SelectedPreset = RainySeasonPreset;
        break;
    case ESeasonType::DrySeason:
        SelectedPreset = DrySeasonPreset;
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("SetWeatherBySeason: Unknown season type"));
        return;
    }

    if (SelectedPreset)
    {
        ApplyWeather(SelectedPreset, TransitionTime);
        UE_LOG(LogTemp, Log, TEXT("SetWeatherBySeason: Weather set for season %s"), *UEnum::GetValueAsString(Season));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SetWeatherBySeason: Preset is not assigned for season %s"), *UEnum::GetValueAsString(Season));
    }
}

AEnvironmentalModifierVolume* AEnvironmentManager::SpawnEnvModVol(
    const FVector& Location,
    float Radius,
    float Height,
    float TempDelta,
    float HumidDelta,
    float ChangeSpeed,
    float Duration
)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEnvironmentalModifierVolume* Volume = World->SpawnActor<AEnvironmentalModifierVolume>(
        AEnvironmentalModifierVolume::StaticClass(),
        Location,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (Volume)
    {
        Volume->Initialize(Radius, Height, TempDelta, HumidDelta, ChangeSpeed, Duration);
        if (Duration > 0)
        {
            FTimerHandle TimerHandle;
            World->GetTimerManager().SetTimer(
                TimerHandle,
                [Volume]()
                {
                    if (IsValid(Volume))
                    {
                        Volume->Destroy();
                    }
                },
                Duration,
                false
            );
        }
    }

    return Volume;
}

void AEnvironmentManager::SetWorldTimeMultiplier_Implementation(int32 Multiplier)
{
    TimeManager->SetWorldTimeMultiplier(Multiplier);
}

void AEnvironmentManager::AdvanceTimeOfDay(float TimeOfDay)
{
    if (!TimeManager) return;

    const float CurrentTimeOfDay = GetCurrentTimeOfDay();
    const int32 TotalSecondsInDay = TimeManager->GetDayCycleLength() * 60;

    float DeltaTimeOfDay = TimeOfDay - CurrentTimeOfDay;
    if (DeltaTimeOfDay < 0.0f)
    {
        DeltaTimeOfDay += 2400.0f;
    }

    const float TimeRatio = DeltaTimeOfDay / 2400.0f;
    const int32 TotalAdvanceSeconds = FMath::RoundToInt(TimeRatio * TotalSecondsInDay);

    const int32 MinuteOffset = TotalAdvanceSeconds / 60;
    const int32 DayOffset = MinuteOffset / (TimeManager->GetDayCycleLength());

    const int32 RemainingMinutes = MinuteOffset % (TimeManager->GetDayCycleLength());

    TimeManager->ModifyTime(DayOffset, RemainingMinutes);
}
