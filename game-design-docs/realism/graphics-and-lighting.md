# Graphics and Lighting for High-Realism Games

## Why it matters

Players read realism first through light. Before they judge animation or physics, they judge whether a surface looks like metal or plastic, whether a room is lit by the window or by a floating fill light, and whether the image looks like it came through a real lens. Most of the jump in visual fidelity over the last decade came from three changes: physically based materials, indirect lighting that responds to the scene, and a camera model that behaves like film or a sensor. Each replaces an artist-tuned hack with a measurable quantity, which also makes scenes hold up under any lighting.

## Core techniques

### PBR materials

Physically based rendering means shading with energy-conserving BRDFs and authoring in measured units. The common model is metallic-roughness with a GGX specular lobe, Smith-style geometric shadowing and Schlick Fresnel. Epic's UE4 notes are the standard reference, and Frostbite's course notes cover the full pipeline.

Implementation notes:
- Author albedo with no baked lighting. Non-metal albedo should stay roughly within 30-240 sRGB (charcoal to fresh snow); true black and white do not occur in nature.
- Metals have coloured F0 and no diffuse. Dielectrics use F0 near 0.04. Keep metalness nearly binary, with intermediate values only for dirt and edge transitions.
- Treat roughness as the main storytelling channel. Fingerprints, wear and wet patches are mostly roughness variation, not colour.
- Work in photometric units (lux, nits, EV) and a linear HDR buffer, so sunlit and shaded surfaces differ by the real ratio, roughly 100,000 lux against a few thousand.
- Add clear-coat, anisotropy, sheen and subsurface lobes for car paint, brushed metal, cloth and skin. Skin needs screen-space or separable subsurface scattering (Jimenez) plus a thin oily specular layer.
- Filter normal maps against roughness (Toksvig or LEAN-style) to stop specular shimmer on distant surfaces.

### Global illumination

Indirect light is what makes a room feel like one space. Options, from cheapest to most costly:
- Baked lightmaps and probes. Highest quality per millisecond, but static. Still the best choice for fixed-time-of-day games and mobile.
- Probe grids with runtime relighting handle dynamic objects and moderate scene changes.
- Voxel or SDF tracing, then screen-space methods, fill the gap on older hardware.
- Lumen in Unreal Engine 5 uses software tracing against signed distance fields and a surface cache, or hardware ray tracing when available, with a radiance-cache final gather. Epic targets roughly 8 ms for GI plus reflections at 1080p internal resolution on current consoles, upscaled with Temporal Super Resolution.

Whatever the method, add ambient occlusion and specular occlusion tied to the GI result. Without them, indirect light leaks into corners and shiny objects glow in the dark.

### Ray and path tracing

Hybrid renderers rasterise primary visibility and ray trace reflections, shadows and AO. Full path tracing, as in Cyberpunk 2077's Overdrive mode, Alan Wake 2 and Portal RTX, traces multiple bounces per pixel and handles direct light, GI and reflections in one framework.

Real-time path tracing is practical only because of denoising and sample reuse. ReSTIR (Bitterli et al., 2020) resamples light candidates across space and time, so thousands of lights cost a few samples per pixel. Pair it with a spatiotemporal denoiser and keep one to two samples per pixel, at full or reduced resolution with upscaling. Plan for ghosting on fast lighting changes and for noise in dark, indirect-only areas.

### Photogrammetry and scanned assets

Photogrammetry captures real surfaces as geometry and albedo. DICE's Star Wars Battlefront (2015) built much of its environment from scans, with textures free of baked shadow. Alan Wake 2 and Hellblade also lean on scanned assets, and Epic's Quixel Megascans library made the workflow standard.

Notes:
- De-light everything. Shoot under overcast skies or with cross-polarised flash, and remove residual shading using a neutral grey reference.
- Retopologise, or use virtualised geometry (Nanite) to avoid hand-built LODs on dense scans.
- Calibrate scanned albedo against a colour chart so assets from different sessions match.
- Scan variety, not just hero objects. Ten scanned rocks reused with variation read as less repetitive than one perfect rock.

### Camera and post-processing

A game camera should behave like a real one, because viewers use cinema and photography as their reference for "real".
- Exposure: drive auto-exposure from a histogram, adapt slowly (a few seconds from bright to dark, faster dark to bright), and expose for the subject, not the average. Use a filmic tone mapper with a gentle shoulder so highlights roll off instead of clipping.
- Depth of field: model a physical aperture and focal length. Bokeh shape and highlight bloom sell scale. Use a gather-based method with a near-field pass to avoid halos.
- Motion blur: per-object plus camera velocity buffers, with the shutter angle near 180 degrees (half a frame of exposure) as in film. McGuire et al.'s reconstruction filter is the usual starting point. Jimenez's Call of Duty: Advanced Warfare talk shows a production-quality approach.
- Lens effects: bloom from a physical glare kernel, restrained chromatic aberration, vignette and subtle film grain. Apply grain after tone mapping and before UI.
- Volumetrics: froxel-based fog and light shafts give depth cues that no texture can supply.

## Notable examples

- Red Dead Redemption 2: sky, volumetric clouds and time-of-day lighting with strong atmospheric perspective.
- The Last of Us Part II and Uncharted 4 (Naughty Dog): carefully tuned baked lighting and strong material work on a fixed lighting model.
- Cyberpunk 2077 (Overdrive) and Alan Wake 2: real-time path tracing at the high end.
- The Matrix Awakens: Lumen, Nanite and MetaHuman together in a dense city.
- Star Wars Battlefront: photogrammetry plus Frostbite PBR.
- Forza Horizon and Gran Turismo: car paint, HDR and camera simulation.

## Trade-offs

- Accuracy against art direction. A physically correct scene can look flat or muddy. Cinematographers cheat with rim lights and fill. Offer an art-direction layer, such as light-linking and per-light GI multipliers, rather than breaking the physical model globally.
- Cost. Path tracing is a large frame-time hit even on top-end GPUs and is gated behind upscalers and frame generation. Frame generation adds latency, which matters in competitive and fast action games.
- Readability. Deep darkness, heavy motion blur and shallow DoF hurt gameplay legibility. Ship sliders for blur and grain, and keep gameplay-critical objects readable after tone mapping.
- Memory. Scan-based assets and virtual geometry push storage into tens of gigabytes. Budget texture streaming early.

## Common pitfalls

- Over-bright or crushed albedo, which breaks energy conservation in lighting.
- Baked shadows left in scanned textures, causing double shadows once lit.
- Constant roughness across a surface. Real surfaces always vary.
- GI light leaking through thin walls, or missing contact shadows that make objects float.
- Over-applied post effects: heavy bloom, grain and chromatic aberration read as "game" rather than "camera".
- Mixing authored-lit and physically lit assets, which makes some objects look pasted in.
- Skin and eyes that miss subsurface scattering, wet specular or correct highlights land in the uncanny valley; see the animation document for the motion side.

## Sources

- Karis, Real Shading in Unreal Engine 4 (SIGGRAPH 2013): https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
- Lagarde and de Rousiers, Moving Frostbite to PBR (SIGGRAPH 2014): https://www.ea.com/frostbite/news/moving-frostbite-to-pb
- SIGGRAPH Physically Based Shading in Theory and Practice, 2014 course: https://blog.selfshadow.com/publications/s2014-shading-course/
- Lazarov, Physically Based Shading in Call of Duty: Black Ops II (2013): https://blog.selfshadow.com/publications/s2013-shading-course/lazarov/s2013_pbs_black_ops_2_slides_v2.pdf
- Wright, Lumen: Real-time Global Illumination in Unreal Engine 5 (SIGGRAPH 2022): https://advances.realtimerendering.com/s2022/index.html
- Epic, Lumen Technical Details: https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine
- Epic, Unreal Engine 5 goes all-in on dynamic global illumination with Lumen: https://www.unrealengine.com/tech-blog/unreal-engine-5-goes-all-in-on-dynamic-global-illumination-with-lumen
- Bitterli et al., Spatiotemporal reservoir resampling (ReSTIR): https://research.nvidia.com/publication/2020-07_spatiotemporal-reservoir-resampling-real-time-ray-tracing-dynamic-direct
- Jimenez, Next Generation Post Processing in Call of Duty: Advanced Warfare (SIGGRAPH 2014): https://advances.realtimerendering.com/s2014/index.html
- McGuire et al., A Reconstruction Filter for Plausible Motion Blur: http://casual-effects.com/research/McGuire2012Blur/index.html
- Real-Time Rendering resources: https://www.realtimerendering.com/
