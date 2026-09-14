#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
traffic_h = (root / 'Source/GTT/Public/Traffic/GTTTrafficCarPawn.h').read_text(encoding='utf-8')
traffic_cpp = (root / 'Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp').read_text(encoding='utf-8')
incident_h = (root / 'Source/GTT/Public/Vehicles/GTTNativeRoadIncidentSubsystem.h').read_text(encoding='utf-8')
incident_cpp = (root / 'Source/GTT/Private/Vehicles/GTTNativeRoadIncidentSubsystem.cpp').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')

required = [
    (traffic_h + traffic_cpp, 'RegisterCollisionIncident'),
    (traffic_h + traffic_cpp, 'ReactToNearbyIncident'),
    (traffic_cpp, 'TRAFFIC_CRASH_RESPONSE'),
    (traffic_cpp, 'EffectiveCruiseSpeedCm'),
    (incident_h + incident_cpp, 'FActiveTrafficIncident'),
    (incident_h + incident_cpp, 'UpdateActiveIncidents'),
    (incident_cpp, 'NATIVE_ROAD_HIT_AND_RUN'),
    (incident_cpp, 'HitAndRunEscapeRadiusCm'),
    (incident_cpp, 'NearbyReactionRadiusCm'),
    (workflow, 'Verify Native road crash-scene AI and hit-and-run escalation'),
    (workflow, 'python Scripts/verify_native_road_crash_scene.py'),
    (roadmap, '<!-- SWIR-ROADMAP-STANDARD:v1 -->'),
]
missing = [token for text, token in required if token not in text]
if missing:
    raise SystemExit('Missing required tokens: ' + ', '.join(missing))
print('[OK] Native road crash-scene AI and hit-and-run contract verified.')
