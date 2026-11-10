#include "World/DelhiAtmosphereTypes.h"

#include "DelhiPopulationTypes.h"

// ---------------------------------------------------------------------------------------------- helpers
namespace
{
	constexpr double kPi = 3.14159265358979323846;
	inline double Rad(double D) { return D * kPi / 180.0; }
	inline double Deg(double R) { return R * 180.0 / kPi; }

	double JulianDay(const FDateTime& Utc)
	{
		int32 Y = Utc.GetYear();
		int32 M = Utc.GetMonth();
		const double D = Utc.GetDay();
		if (M <= 2) { Y -= 1; M += 12; }
		const int32 A = Y / 100;
		const int32 B = 2 - A + A / 4;
		const double Jd0 = FMath::FloorToDouble(365.25 * (Y + 4716)) + FMath::FloorToDouble(30.6001 * (M + 1)) + D + B - 1524.5;
		return Jd0 + Utc.GetTimeOfDay().GetTotalDays();
	}

	FDelhiAtmosphereState Mk(float R, float Ms, float Ma, float G, float H,
	                         float FogD, float FogF, FLinearColor FogI, float VExt,
	                         float AOD, float Cloud, float TempK, float Diffuse, float EV,
	                         float Wet, float Dust, float Smog, float Wind, float Rain, float Vis, float Sat)
	{
		FDelhiAtmosphereState S;
		S.RayleighScale = R; S.MieScatteringScale = Ms; S.MieAbsorptionScale = Ma; S.MieAnisotropy = G; S.MieScaleHeightKm = H;
		S.FogDensity = FogD; S.FogHeightFalloff = FogF; S.FogInscattering = FogI; S.VolumetricFogExtinctionScale = VExt;
		S.AOD = AOD; S.CloudCover = Cloud; S.SunTemperatureK = TempK; S.DiffuseNoonLux = Diffuse; S.BaseEV100 = EV;
		S.Wetness = Wet; S.Dust = Dust; S.Smog = Smog; S.WindStrength = Wind; S.RainIntensity = Rain;
		S.VisibilityMeters = Vis; S.Saturation = Sat;
		return S;
	}
}

// ---------------------------------------------------------------------------------------------- state lerp
FDelhiAtmosphereState FDelhiAtmosphereState::Lerp(const FDelhiAtmosphereState& A, const FDelhiAtmosphereState& B, float T)
{
	FDelhiAtmosphereState R;
#define L(F) R.F = FMath::Lerp(A.F, B.F, T)
	L(RayleighScale); L(MieScatteringScale); L(MieAbsorptionScale); L(MieAnisotropy); L(MieScaleHeightKm);
	L(FogDensity); L(FogHeightFalloff); L(VolumetricFogExtinctionScale);
	L(AOD); L(CloudCover); L(SunTemperatureK); L(DiffuseNoonLux); L(BaseEV100);
	L(Wetness); L(Dust); L(Smog); L(WindStrength); L(RainIntensity); L(Saturation);
#undef L
	R.FogInscattering = FMath::Lerp(A.FogInscattering, B.FogInscattering, T);
	// Visibility is perceptually logarithmic.
	R.VisibilityMeters = FMath::Exp(FMath::Lerp(FMath::Loge(FMath::Max(1.f, A.VisibilityMeters)), FMath::Loge(FMath::Max(1.f, B.VisibilityMeters)), T));
	return R;
}

// ---------------------------------------------------------------------------------------------- cities
FAtmosphereCityGeo UDelhiAtmosphereLibrary::GetCityGeo(EAtmosphereCity City)
{
	FAtmosphereCityGeo G;
	if (City == EAtmosphereCity::Karachi)
	{
		G.Name = TEXT("Karachi"); G.Latitude = 24.87f; G.Longitude = 66.99f; G.UtcOffsetHours = 5.0f; // PKT, no DST
	}
	else
	{
		G.Name = TEXT("Delhi"); G.Latitude = 28.656f; G.Longitude = 77.232f; G.UtcOffsetHours = 5.5f; // IST (A4.2: set explicitly)
	}
	return G;
}

// ---------------------------------------------------------------------------------------------- presets
// Numbers from research/02 B2.6 (sun, EV) and B2.7 (sky). All [E]: tune against reference photos in-engine.
FDelhiAtmosphereState UDelhiAtmosphereLibrary::GetBaseState(EDelhiWeather Weather, EAtmosphereCity City)
{
	const FLinearColor Grey(0.50f, 0.54f, 0.62f), Warm(0.50f, 0.45f, 0.36f), Brown(0.42f, 0.36f, 0.30f);
	FDelhiAtmosphereState S;
	switch (Weather)
	{
	//                R     Ms    Ma    g     H     fogD   fogF   inscatter                  vExt AOD  cloud T     diffuse EV   wet  dust smog wind rain vis    sat
	case EDelhiWeather::Clear:
		S = Mk(1.0f, 1.0f, 1.0f, 0.80f, 1.2f, 0.010f, 0.20f, Grey, 1.0f, 0.7f, 0.05f, 5900, 30000, 14.5f, 0.f, 0.10f, 0.15f, 2.f, 0.f, 20000, 1.0f); break;
	case EDelhiWeather::Haze:
		S = Mk(0.9f, 5.0f, 3.0f, 0.70f, 2.0f, 0.020f, 0.10f, Warm, 1.5f, 1.2f, 0.15f, 5600, 35000, 14.0f, 0.f, 0.25f, 0.45f, 2.f, 0.f, 6000, 0.95f); break;
	case EDelhiWeather::Smog:
		// between "typical" (AOD 2.0, 12.0-12.5 EV) and "severe" (AOD 3.0, 11.0-11.5 EV), AQI 400ish
		S = Mk(0.5f, 10.0f, 6.5f, 0.65f, 1.2f, 0.120f, 0.07f, Brown, 2.5f, 2.5f, 0.20f, 4200, 9000, 11.75f, 0.f, 0.30f, 0.90f, 1.f, 0.f, 800, 0.80f); break;
	case EDelhiWeather::Fog:
		S = Mk(1.0f, 7.0f, 0.0f, 0.40f, 0.4f, 0.350f, 0.30f, FLinearColor(0.75f, 0.78f, 0.82f), 3.0f, 1.0f, 0.95f, 6500, 7000, 10.75f, 0.2f, 0.10f, 1.00f, 0.5f, 0.f, 150, 0.85f); break;
	case EDelhiWeather::Rain:
		S = Mk(1.0f, 2.0f, 0.5f, 0.80f, 1.5f, 0.030f, 0.10f, Grey, 1.5f, 0.35f, 0.85f, 6500, 20000, 12.8f, 0.8f, 0.0f, 0.20f, 5.f, 0.55f, 3000, 0.92f); break;
	case EDelhiWeather::Monsoon:
		S = Mk(1.0f, 2.0f, 0.5f, 0.80f, 1.5f, 0.035f, 0.10f, Grey, 1.8f, 0.35f, 0.95f, 6500, 18000, 12.5f, 1.0f, 0.0f, 0.15f, 8.f, 0.90f, 1500, 0.90f); break;
	case EDelhiWeather::DustStorm: // aandhi
		S = Mk(0.8f, 15.0f, 10.0f, 0.60f, 3.5f, 0.060f, 0.05f, FLinearColor(0.55f, 0.36f, 0.20f), 3.0f, 2.5f, 0.30f, 4500, 12000, 11.5f, 0.f, 1.0f, 0.60f, 25.f, 0.f, 300, 0.85f); break;
	case EDelhiWeather::Overcast:
		S = Mk(1.0f, 2.0f, 0.5f, 0.80f, 1.5f, 0.025f, 0.10f, Grey, 1.2f, 0.6f, 0.75f, 6300, 22000, 13.2f, 0.f, 0.05f, 0.25f, 3.f, 0.f, 12000, 0.95f); break;
	case EDelhiWeather::Thunderstorm:
		S = Mk(1.0f, 3.0f, 1.0f, 0.75f, 1.5f, 0.045f, 0.10f, Grey, 2.0f, 0.5f, 1.0f, 6500, 9000, 11.5f, 1.0f, 0.0f, 0.10f, 12.f, 1.0f, 1200, 0.88f); break;
	case EDelhiWeather::SeaHaze:
		S = Mk(1.0f, 3.5f, 1.2f, 0.75f, 1.6f, 0.025f, 0.12f, FLinearColor(0.55f, 0.58f, 0.62f), 1.5f, 0.9f, 0.20f, 5800, 32000, 14.2f, 0.f, 0.15f, 0.25f, 4.f, 0.f, 8000, 0.97f); break;
	}
	if (City == EAtmosphereCity::Karachi)
	{
		// Coastal: more humid scattering at the horizon, a little less dust, brisker sea breeze.
		S.MieScatteringScale *= 1.1f;
		S.Dust *= 0.8f;
		if (Weather == EDelhiWeather::Clear || Weather == EDelhiWeather::Haze || Weather == EDelhiWeather::SeaHaze) { S.WindStrength = FMath::Max(S.WindStrength, 4.f); }
	}
	return S;
}

// ---------------------------------------------------------------------------------------------- sun
void UDelhiAtmosphereLibrary::ComputeSunPosition(const FDateTime& LocalTime, float LatitudeDeg, float LongitudeDeg, float UtcOffsetHours,
                                                 float& OutAzimuthDeg, float& OutElevationDeg, bool bApplyRefraction)
{
	const FDateTime Utc = LocalTime - FTimespan::FromHours(UtcOffsetHours);
	const double Jd = JulianDay(Utc);
	const double Jc = (Jd - 2451545.0) / 36525.0;

	const double L0 = FMath::Fmod(280.46646 + Jc * (36000.76983 + Jc * 0.0003032), 360.0);
	const double M = 357.52911 + Jc * (35999.05029 - 0.0001537 * Jc);
	const double E = 0.016708634 - Jc * (0.000042037 + 0.0000001267 * Jc);
	const double C = FMath::Sin(Rad(M)) * (1.914602 - Jc * (0.004817 + 0.000014 * Jc))
		+ FMath::Sin(Rad(2 * M)) * (0.019993 - 0.000101 * Jc) + FMath::Sin(Rad(3 * M)) * 0.000289;
	const double TrueLong = L0 + C;
	const double AppLong = TrueLong - 0.00569 - 0.00478 * FMath::Sin(Rad(125.04 - 1934.136 * Jc));
	const double MeanObliq = 23.0 + (26.0 + ((21.448 - Jc * (46.815 + Jc * (0.00059 - Jc * 0.001813)))) / 60.0) / 60.0;
	const double Obliq = MeanObliq + 0.00256 * FMath::Cos(Rad(125.04 - 1934.136 * Jc));
	const double Decl = FMath::Asin(FMath::Sin(Rad(Obliq)) * FMath::Sin(Rad(AppLong)));

	const double Y = FMath::Square(FMath::Tan(Rad(Obliq) / 2.0));
	const double EqTime = 4.0 * Deg(Y * FMath::Sin(2 * Rad(L0)) - 2 * E * FMath::Sin(Rad(M))
		+ 4 * E * Y * FMath::Sin(Rad(M)) * FMath::Cos(2 * Rad(L0))
		- 0.5 * Y * Y * FMath::Sin(4 * Rad(L0)) - 1.25 * E * E * FMath::Sin(2 * Rad(M)));

	const double UtcMinutes = Utc.GetTimeOfDay().GetTotalMinutes();
	double Tst = FMath::Fmod(UtcMinutes + EqTime + 4.0 * LongitudeDeg, 1440.0);
	if (Tst < 0.0) { Tst += 1440.0; }
	const double Ha = Tst / 4.0 - 180.0;

	const double Lat = Rad(LatitudeDeg);
	const double CosZen = FMath::Clamp(FMath::Sin(Lat) * FMath::Sin(Decl) + FMath::Cos(Lat) * FMath::Cos(Decl) * FMath::Cos(Rad(Ha)), -1.0, 1.0);
	const double Zen = FMath::Acos(CosZen);
	double Elev = 90.0 - Deg(Zen);

	double Az = 0.0;
	const double SinZen = FMath::Sin(Zen);
	if (FMath::Abs(SinZen) > 1e-6)
	{
		const double CosAz = FMath::Clamp((FMath::Sin(Lat) * CosZen - FMath::Sin(Decl)) / (FMath::Cos(Lat) * SinZen), -1.0, 1.0);
		const double AzR = Deg(FMath::Acos(CosAz));
		Az = (Ha > 0.0) ? FMath::Fmod(AzR + 180.0, 360.0) : FMath::Fmod(540.0 - AzR, 360.0);
	}

	if (bApplyRefraction && Elev > -0.575 && Elev < 85.0)
	{
		const double T = FMath::Tan(Rad(Elev));
		double Arcsec;
		if (Elev > 5.0) { Arcsec = 58.1 / T - 0.07 / (T * T * T) + 0.000086 / FMath::Pow(T, 5.0); }
		else { Arcsec = 1735.0 + Elev * (-518.2 + Elev * (103.4 + Elev * (-12.79 + Elev * 0.711))); }
		Elev += Arcsec / 3600.0;
	}

	OutAzimuthDeg = (float)Az;
	OutElevationDeg = (float)Elev;
}

void UDelhiAtmosphereLibrary::ComputeMoon(const FDateTime& LocalTime, float LatitudeDeg, float LongitudeDeg, float UtcOffsetHours,
                                          float& OutAzimuthDeg, float& OutElevationDeg, float& OutIlluminatedFraction)
{
	constexpr double Synodic = 29.530588853;
	const FDateTime Utc = LocalTime - FTimespan::FromHours(UtcOffsetHours);
	double Age = FMath::Fmod(JulianDay(Utc) - 2451550.1, Synodic); // new moon 2000-01-06 18:14 UTC
	if (Age < 0.0) { Age += Synodic; }
	OutIlluminatedFraction = (float)(0.5 * (1.0 - FMath::Cos(2.0 * kPi * Age / Synodic)));
	// The moon trails the sun by (age/synodic) of a day in hour angle.
	const FDateTime Shifted = LocalTime - FTimespan::FromHours(Age / Synodic * 24.0);
	ComputeSunPosition(Shifted, LatitudeDeg, LongitudeDeg, UtcOffsetHours, OutAzimuthDeg, OutElevationDeg, true);
}

FRotator UDelhiAtmosphereLibrary::LightRotationFromSun(float AzimuthDeg, float ElevationDeg, float NorthYawDegrees)
{
	// Yaw grows clockwise seen from above, like a compass. Light travels away from the body.
	return FRotator(-ElevationDeg, NorthYawDegrees + AzimuthDeg + 180.f, 0.f);
}

float UDelhiAtmosphereLibrary::CloudBeamFactor(float CloudCover)
{
	return 1.f - 0.95f * FMath::Pow(FMath::Clamp(CloudCover, 0.f, 1.f), 1.3f);
}

float UDelhiAtmosphereLibrary::EstimateDirectSunLux(float ElevationDeg, float AOD, float CloudCover)
{
	if (ElevationDeg <= 0.f) { return 0.f; }
	const double E = Rad(ElevationDeg);
	// Kasten-Young air mass (stays finite at the horizon).
	const double Air = 1.0 / (FMath::Sin(E) + 0.50572 * FMath::Pow(ElevationDeg + 6.07995, -1.6364));
	const double Tau = 0.20 + AOD; // tau_clean 0.18-0.22 + aerosol
	return (float)(127500.0 * FMath::Exp(-Tau * Air)) * CloudBeamFactor(CloudCover);
}

float UDelhiAtmosphereLibrary::DayFactor(float El)
{
	return FMath::SmoothStep(-6.f, 4.f, El);
}

float UDelhiAtmosphereLibrary::EstimateExposureEV100(const FDelhiAtmosphereState& S, float El, float NightEV100)
{
	auto Illum = [&S](float Elev)
	{
		const float Sn = FMath::Sin(FMath::DegreesToRadians(FMath::Max(Elev, 0.f)));
		const float Direct = EstimateDirectSunLux(Elev, S.AOD, S.CloudCover) * Sn;
		const float Diffuse = S.DiffuseNoonLux * (0.25f + 0.75f * Sn) * FMath::SmoothStep(-8.f, 2.f, Elev);
		return Direct + Diffuse;
	};
	const float Ref = FMath::Max(1.f, Illum(85.f));
	const float Ratio = FMath::Max(Illum(El) / Ref, 0.002f);
	const float EvDay = S.BaseEV100 + FMath::Log2(Ratio);
	return FMath::Lerp(NightEV100, FMath::Max(EvDay, NightEV100), DayFactor(El));
}

// ---------------------------------------------------------------------------------------------- climate
FDelhiClimateMonthRow UDelhiAtmosphereLibrary::GetBuiltinClimate(EAtmosphereCity City, int32 Month)
{
	// {Clear, Haze, Overcast, Rain, Storm, Fog, Smog, Persistence}
	static const float Delhi[12][8] = {
		{0.22f, 0.15f, 0.10f, 0.06f, 0.00f, 0.30f, 0.17f, 0.80f}, {0.35f, 0.20f, 0.10f, 0.05f, 0.02f, 0.15f, 0.13f, 0.65f},
		{0.50f, 0.20f, 0.10f, 0.06f, 0.06f, 0.03f, 0.05f, 0.50f}, {0.55f, 0.20f, 0.06f, 0.03f, 0.12f, 0.00f, 0.04f, 0.50f},
		{0.45f, 0.27f, 0.05f, 0.08f, 0.15f, 0.00f, 0.00f, 0.50f}, {0.25f, 0.25f, 0.22f, 0.16f, 0.12f, 0.00f, 0.00f, 0.55f},
		{0.08f, 0.17f, 0.42f, 0.31f, 0.02f, 0.00f, 0.00f, 0.65f}, {0.08f, 0.17f, 0.40f, 0.33f, 0.02f, 0.00f, 0.00f, 0.65f},
		{0.22f, 0.28f, 0.30f, 0.18f, 0.02f, 0.00f, 0.00f, 0.60f}, {0.45f, 0.35f, 0.08f, 0.03f, 0.00f, 0.00f, 0.09f, 0.65f},
		{0.17f, 0.20f, 0.03f, 0.02f, 0.00f, 0.20f, 0.38f, 0.80f}, {0.15f, 0.15f, 0.03f, 0.02f, 0.00f, 0.35f, 0.30f, 0.80f}};
	// Karachi is not in the research tables: marine climate, rain only Jun-Sep, pre-monsoon dust [E].
	static const float Karachi[12][8] = {
		{0.45f, 0.30f, 0.08f, 0.03f, 0.00f, 0.06f, 0.08f, 0.60f}, {0.48f, 0.30f, 0.08f, 0.03f, 0.00f, 0.04f, 0.07f, 0.60f},
		{0.45f, 0.38f, 0.08f, 0.02f, 0.02f, 0.00f, 0.05f, 0.55f}, {0.35f, 0.50f, 0.08f, 0.00f, 0.05f, 0.00f, 0.02f, 0.55f},
		{0.30f, 0.55f, 0.08f, 0.00f, 0.05f, 0.00f, 0.02f, 0.55f}, {0.15f, 0.55f, 0.20f, 0.03f, 0.07f, 0.00f, 0.00f, 0.60f},
		{0.08f, 0.45f, 0.30f, 0.12f, 0.05f, 0.00f, 0.00f, 0.65f}, {0.08f, 0.45f, 0.32f, 0.10f, 0.05f, 0.00f, 0.00f, 0.65f},
		{0.20f, 0.50f, 0.20f, 0.04f, 0.06f, 0.00f, 0.00f, 0.60f}, {0.35f, 0.50f, 0.08f, 0.01f, 0.02f, 0.00f, 0.04f, 0.60f},
		{0.40f, 0.38f, 0.05f, 0.02f, 0.00f, 0.05f, 0.10f, 0.65f}, {0.48f, 0.30f, 0.06f, 0.03f, 0.00f, 0.06f, 0.07f, 0.60f}};
	const float* R = (City == EAtmosphereCity::Karachi ? Karachi : Delhi)[FMath::Clamp(Month, 1, 12) - 1];
	FDelhiClimateMonthRow Row;
	Row.Clear = R[0]; Row.Haze = R[1]; Row.Overcast = R[2]; Row.Rain = R[3]; Row.Storm = R[4]; Row.Fog = R[5]; Row.Smog = R[6]; Row.Persistence = R[7];
	return Row;
}

EDelhiWeather UDelhiAtmosphereLibrary::ResolveDailyWeather(EDelhiDailyState Daily, EAtmosphereCity City, int32 Month, bool bInEpisode, float Hour, float FogBurnOffHour)
{
	const bool bKarachi = City == EAtmosphereCity::Karachi;
	const bool bMonsoon = Month >= 6 && Month <= 9;
	const bool bDustSeason = bKarachi ? (Month >= 4 && Month <= 6) : (Month >= 3 && Month <= 6);
	switch (Daily)
	{
	case EDelhiDailyState::Clear:    return EDelhiWeather::Clear;
	case EDelhiDailyState::Haze:     return bKarachi ? EDelhiWeather::SeaHaze : EDelhiWeather::Haze;
	case EDelhiDailyState::Overcast: return EDelhiWeather::Overcast;
	case EDelhiDailyState::Rain:
		return bInEpisode ? (bMonsoon ? EDelhiWeather::Monsoon : EDelhiWeather::Rain) : EDelhiWeather::Overcast;
	case EDelhiDailyState::Storm:
		if (bInEpisode) { return bDustSeason ? EDelhiWeather::DustStorm : EDelhiWeather::Thunderstorm; }
		return bDustSeason ? (bKarachi ? EDelhiWeather::SeaHaze : EDelhiWeather::Haze) : EDelhiWeather::Overcast;
	case EDelhiDailyState::Fog:
		if (bKarachi) { return EDelhiWeather::SeaHaze; }
		return (Hour < FogBurnOffHour || Hour > 22.f) ? EDelhiWeather::Fog : EDelhiWeather::Haze;
	case EDelhiDailyState::Smog:     return bKarachi ? EDelhiWeather::Haze : EDelhiWeather::Smog;
	}
	return EDelhiWeather::Clear;
}

float UDelhiAtmosphereLibrary::PrevailingWindFromBearing(EAtmosphereCity City, int32 Month, EDelhiWeather Weather)
{
	if (Weather == EDelhiWeather::DustStorm) { return City == EAtmosphereCity::Karachi ? 250.f : 290.f; } // aandhi from the W/NW
	if (City == EAtmosphereCity::Karachi)
	{
		if (Month >= 4 && Month <= 9) { return 225.f; } // SW sea breeze / monsoon flow
		return 30.f;                                    // NNE winter land breeze
	}
	if (Month >= 11 || Month <= 2) { return 315.f; }    // NW winter
	if (Month >= 3 && Month <= 6) { return 270.f; }     // W pre-monsoon loo
	if (Month >= 7 && Month <= 9) { return 100.f; }     // E/SE monsoon
	return 300.f;                                       // October
}
