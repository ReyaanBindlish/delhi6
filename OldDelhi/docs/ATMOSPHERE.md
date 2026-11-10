# Atmosphere, weather and world boundary

Owner: atmosphere agent. Code: `Source/OldDelhi/World/*`, `Source/OldDelhi/DelhiWeatherSubsystem.*`, `Tools/ue/setup_lighting.py`.
Numbers come from research/02 (A1.2 sun table, A4.2 clock, A4.3 climatology, B2.6 exposure, B2.7 sky) and implementation.md 4.2. Every `[E]` value there is still an estimate; tune in engine against reference photos.

## 1. Sun and moon (real position)

`UDelhiAtmosphereLibrary::ComputeSunPosition` is the NOAA solar-position algorithm (Julian century, equation of time, declination, hour angle, refraction). Inputs are the local wall-clock time from `UStoryCalendarSubsystem`, latitude, longitude and an explicit UTC offset.

| City | Lat N | Lon E | UTC offset |
|---|---|---|---|
| Delhi | 28.656 | 77.232 | +5.5 (IST) |
| Karachi | 24.87 | 66.99 | +5.0 (PKT) |

Checks run against the Python port (same maths): Delhi 15 Nov noon gives elevation 42.9 (research table: 43); Delhi 15 Jun 12:00 IST gives 82.8 (solar noon at 12:11-12:20 gives about 84.6).

Frame: the project is X east, Y south, so true north is yaw -90 (`NorthYawDegrees`, present on the rig and on the weather subsystem; keep them equal). A light for a body at azimuth az / elevation el is `Rotator(-el, NorthYaw + az + 180, 0)`.

The moon is placed from the synodic age (hour angle trails the sun by age/29.53 of a day, no lunar inclination) and lit by phase, 0.25 lux at full. It is the second atmosphere light (index 1), off while the sun is up.

Sun lux, two modes (`ESunIntensityMode`):
- `AtmosphereAttenuated` (default): 127,500 lux top-of-atmosphere times a cloud beam factor `1 - 0.95*cover^1.3`. The Sky Atmosphere applies its own Beer-Lambert, so the research table values emerge from the Mie scale per state. This avoids counting the aerosol attenuation twice. The light stays on through twilight (sky glow) and fades below -16 degrees.
- `ResearchTable`: `E_dn = 127,500 * exp(-(0.2 + AOD) * m) * cloud` (Kasten-Young air mass) used directly. Use for calibration only.

Sun colour temperature per state is the base colour (5900 clear, 4200 smog, 4500 dust); the atmosphere reddens it further at low elevation.

## 2. Weather

`UDelhiWeatherSubsystem` (world subsystem, tickable) owns the clock and the weather.

Clock (A4.2): 30 game seconds per real second by day, 60 at night, smoothstepped through 05:00-07:30 and 17:00-19:00. The old code treated the rates as game minutes per second (a day in 48 s); it is now seconds. It writes the time into `UStoryCalendarSubsystem` (`bDriveStoryCalendar`). The calendar DATE only changes by story beats (A4.2): at midnight the clock wraps inside the day unless `bClockRollsDate`. `SkipHours()` is the sleep/skip entry point and fires `OnDayEnded` through the calendar.

Weather (A4.3): per day, a Markov chain over `DT_ClimateMonth.csv` (row names Jan..Dec, struct `FDelhiClimateMonthRow`). Stay in yesterday's state with probability `Persistence` (only if it is possible this month), otherwise draw from the month. The table is auto-loaded from `/Game/Delhi/Data/DT_ClimateMonth` or `/Game/Data/DT_ClimateMonth`; without it a built-in copy of the CSV is used. Karachi has no research table, so a built-in estimate [E] is used (override with `KarachiClimateTable`).

Daily state to weather within the day:
- Rain: one episode of 3.5-8 game hours (starts 06:00-22:00), Monsoon preset in Jun-Sep else Rain, Overcast outside it.
- Storm: 0.8-2.2 h episode in the afternoon/evening. Dust storm (aandhi) in Mar-Jun (Delhi) / Apr-Jun (Karachi), thunderstorm otherwise.
- Fog (Delhi): fog until a burn-off hour drawn in 09:30-11:30, then haze, fog again after 22:00. Karachi swaps fog for sea haze.
- Smog (Delhi) persists all day; Karachi shows haze.

Presets (`GetBaseState`, B2.6/B2.7): Clear, Haze (Delhi summer heat haze and dust), Smog (AQI ~400), Fog, Rain, Monsoon, DustStorm, Overcast, Thunderstorm, SeaHaze (Karachi humid marine haze). Each carries Rayleigh/Mie scales, anisotropy, Mie scale height, fog density/falloff/tint, volumetric fog extinction, AOD, cloud cover, sun colour, diffuse noon lux, EV100, wetness, dust, smog, wind, rain intensity, visibility and saturation. Sky atmosphere scales are relative to the component's own defaults (read from the component at construction), as B2.7 says. Transitions blend over `AutoBlendSeconds` (45 s real, about 10 game minutes by day) from a snapshot of the current blended state, so there is no popping when interrupted.

`SetWeather()` is a manual override until `ReleaseWeatherOverride()`. Events: `OnWeatherChanged`, `OnDailyWeatherRolled`.

### What physics and gameplay read
- `GetWetness()` (0-1, wets at 0.2/s, dries at 0.01/s), `GetTractionMultiplier()` (1 dry to about 0.55 soaked, times a small dust factor), `GetWind()` / `GetWindAtLocation()` (m/s, world space, direction the air moves to; prevailing bearing by city and month, with gusts), `GetRainIntensity()`, `GetDust()`, `GetSmog()`, `GetVisibilityMeters()`, `GetNightFactor()`, `GetSunElevationDeg()`.
- `MPC_Weather` (auto-loaded from `/Game/Delhi/Materials/MPC_Weather`, written at 20 Hz): Wetness, Dust, Smog, WindX, WindY, WindStrength, RainIntensity, Visibility, SunElevation, NightFactor, TimeOfDay. Street lights should key off NightFactor.

## 3. Sky rig (`ADelhiSkyRig`)

One actor, six engine components: Sun and Moon directional lights (atmosphere lights 0 and 1), SkyAtmosphere, SkyLight (real-time capture, lower hemisphere not black), ExponentialHeightFog with volumetric fog, VolumetricCloud, and an unbound PostProcess. Place it at ground level (the fog sits at its Z). Only one per level.

- In game it follows the weather subsystem (time, state, city). In the editor it previews `PreviewYear/Month/Day/Hour` and `PreviewWeather` through `OnConstruction`, and the "Refresh Sky" button.
- Exposure (B2.6): Auto Exposure Histogram, physical camera exposure on, speed up 3 / down 1. Target EV100 comes from the state's noon EV, scaled by the ratio of illuminance at the current elevation to noon, blended to `NightEV100` (3.5) through twilight. The Min/Max band is target -3 / +1.5, clamped to [1, 15]. `ExposureBias` shifts it.
- Heavy updates (atmosphere, fog, post) run every `UpdateIntervalSeconds` (0.2). The atmosphere is re-created only when its values move. The sun rotation updates every frame.
- Cloud coverage is written to the cloud material's `Coverage` scalar if that parameter exists (`CloudCoverageParameter`); the stock `m_SimpleVolumetricCloud_Inst` parameter names may differ, so check once in engine or swap in a project cloud material.
- `FogInscatterScale` exists because physical exposure makes fixed fog luminances read dark or bright; tune it once per project.

## 4. Weather controller (`ADelhiWeatherController`)

Level-placed configuration pushed into the subsystem at BeginPlay: city, start date and hour (applied to the calendar), auto weather, optional forced initial weather (the four Phase 1 test days), seed, MPC, clock rates, pause and time scale. Blueprint hooks: `OnWeatherStateChanged`, `OnDailyStateRolled` (bind rain/dust Niagara and audio zones), plus pass-through getters.

## 5. Construct wall (`AWorldBoundary`, `UBoundarySubsystem`, `M_Boundary`)

`AWorldBoundary`:
- Closed linear spline (default 2.4 km box; edit points, or `SetPolygonFromLatLon` with the Netaji Subhash Marg / railway / S P Mukherjee Marg / Asaf Ali Road outline; `SnapSplineToGround` drops points onto terrain).
- Visual: per-segment `USplineMeshComponent` (engine Cube stretched to 30 m tall and 2 cm thick), translucent, no collision, no shadows, culled beyond 50 m. Each segment gets a dynamic material instance with `Seg` (anchor XY, distance along wall, ground Z) and `SegDir` so the lattice is continuous along curved walls.
- Collision: one box per segment, 1 m thick, 30 m tall, blocking Pawn, Vehicle and PhysicsBody only. Bullets, visibility and camera traces pass. Components are tagged `NoDecal`.
- Wall components are transient and rebuilt on construction and on BeginPlay.

`UBoundarySubsystem` (20 Hz, game and PIE worlds only): writes `MPC_Boundary` (PlayerPos, Proximity, RevealRadius 12 m), finds the nearest boundary, and
- pushes back inside 2 m (characters: velocity change, only when approaching or very close; physics vehicles: impulse scaled by mass and speed),
- fires `OnBoundaryProximity` every update (drive a MetaSound hum and bit-crush) and `OnBoundaryTouched` once per approach at 0.5 m from the face, with a controller rumble,
- drives the boundary's hum (`HumSound`, float input `Proximity`) and a proximity post-process within 3 m (chromatic offset; optional scanline material).
Distances are to the wall face (collision half thickness subtracted).

`M_Boundary` (built by the script, node by node): translucent, unlit, two-sided, pixel-normal-offset refraction. Triangular lattice from three line families at 0/60/120 degrees in a 60 cm cell (the dual of the hex pattern), `SphereMask(WorldPos, MPC_Boundary.PlayerPos, 12 m, hardness 0.4)` reveal, Fresnel rim, 80 cm ground fade, time-stepped row glitch (bands jump 0.37 cell and flash), emissive pale cyan-white, opacity = lattice x reveal x ground x 0.7. Parameters: Intensity, LineWidth, Cell.

Content rule from the spec still applies: fill the skyline beyond the wall with low-detail blockout and distance fog.

## 6. Running it

1. Build the OldDelhi module (this agent adds no new module dependencies; everything is in Engine).
2. Open the level. `UnrealEditor-Cmd.exe OldDelhi.uproject <map> -run=pythonscript -script=Tools/ue/setup_lighting.py -unattended -nop4`
3. Environment: `DELHI_CITY=delhi|karachi`, `DELHI_DATETIME="2019-11-15 16:30"`, `DELHI_WEATHER=Smog`, `DELHI_GROUND_Z`, `DELHI_BOUNDARY=0|1`, `DELHI_BOUNDARY_HALF_M`, `DELHI_KEEP_EXISTING`, `DELHI_FORCE_MATERIALS`.
4. The four reference days: Delhi summer clear (`2019-06-15 12:00`, Clear), winter smog (`2019-11-15 16:30`, Smog), monsoon (`2019-08-15 15:00`, Monsoon), dust storm (`2019-05-20 17:00`, DustStorm).

If the C++ module is not built yet, the script falls back to plain engine actors with the same numbers (static for the chosen moment).

## 7. Open items and risks

- Written without compiling (the editor was not launched). Expect a first-compile pass for engine property names (for example `bEnableVolumetricFog`, post-process override flags in 5.8).
- Python property and enum spellings (`unreal.DelhiWeather.DUST_STORM`, `unreal.AtmosphereCity`) follow UE's snake-case rules; the script tries the alternatives and warns rather than failing.
- `Extend default luminance range in Auto Exposure settings` must be on (default for new 5.x projects) or the EV100 band values are wrong.
- Moon and cloud shadows are simplified; no rain or dust Niagara is included (hook `OnWeatherStateChanged`).
