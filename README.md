# 🚜 Grand Theft Tractor (GTT)

**Grand Theft Tractor** is a comedy open-world action game set in a chaotic countryside. Steal tractors and battered cars, take odd jobs, fish where you probably should not, attend village parties, escape the police, and build your reputation across a living rural sandbox.

> Project status: **Pre-Alpha / Prototype foundation**

## Vision

GTT aims for the freedom and readability of classic 3D open-world games while building its own rural identity: tractors, old cars, farms, forests, lakes, village roads, local NPCs, absurd missions and systemic police chases.

## First playable milestone

- Third-person player controller
- Interaction system
- Enter/exit vehicles
- Tractor and old-car vehicle framework
- Vehicle condition and damage foundation
- Wanted level (0–5)
- Police response foundation
- Small playable village test map
- First mission: **Borrowed Tractor**
- Basic HUD, save/load and economy

## Tech

- Unreal Engine 5
- C++ gameplay core + Blueprint-friendly APIs
- Enhanced Input
- Chaos Vehicles
- GitHub for source control and releases

## Repository layout

```text
GTT/
├── Config/                  # Unreal project configuration
├── Content/                 # Unreal assets (added as the project grows)
├── Docs/                    # Design notes and roadmap
├── Source/GTT/              # Runtime C++ module
├── GTT.uproject
├── CHANGELOG.md
└── README.md
```

## Build notes

1. Install a compatible Unreal Engine 5 release and Visual Studio 2022 with the **Game development with C++** workload.
2. Right-click `GTT.uproject` and generate Visual Studio project files if needed.
3. Build the `GTTEditor` target.
4. Open `GTT.uproject` in Unreal Editor.

The repository intentionally does **not** contain Unreal Engine source code or generated build folders.

## Development rules

- Keep gameplay systems modular and Blueprint-friendly.
- Prefer data-driven configuration for vehicles, police and missions.
- Commit source assets; ignore generated/cache folders.
- Every milestone must leave the project in a usable state.
- Large binary assets should use Git LFS when they arrive.

## Roadmap

See [`Docs/ROADMAP.md`](Docs/ROADMAP.md).

## License / third-party assets

Project-specific licensing will be finalized before public distribution. Third-party assets must retain their own license notices and may not be committed unless redistribution is permitted.

---

**GTT — rural chaos starts here.** 🚜🚓
