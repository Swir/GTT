from pathlib import Path
root=Path(__file__).resolve().parents[1]
h=(root/'Source/GTT/Public/Police/GTTRoadblock.h').read_text(encoding='utf-8')
cpp=(root/'Source/GTT/Private/Police/GTTRoadblock.cpp').read_text(encoding='utf-8')
vehicle=(root/'Source/GTT/Public/Vehicles/GTTVehicleBase.h').read_text(encoding='utf-8')
roadmap=(root/'Docs/ROADMAP.md').read_text(encoding='utf-8')
checks={
 'shared tire damage': 'ApplyTireDamage(TireDamage)' in cpp and 'GetTireIntegrity()' in cpp,
 'shared body damage': 'ApplyVehicleDamage(BodyDamage)' in cpp,
 'tier scaling': '0.18f+ResponseTier*0.08f' in cpp and '1.5f+ResponseTier*1.5f' in cpp,
 'repeat hit cooldown': 'SpikeRepeatCooldownSeconds = 0.75f' in cpp and 'LastSpikedActor' in cpp,
 'consequence telemetry': 'ROADBLOCK_SPIKE_CONSEQUENCE' in cpp and 'tire_before' in cpp and 'tire_after' in cpp,
 'runtime getters': all(x in h for x in ['GetSpikeHitCount','GetLastSpikedVehicleId','GetLastTireIntegrityBefore','GetLastTireIntegrityAfter','HasProvenSpikeConsequence']),
 'vehicle contract': all(x in vehicle for x in ['ApplyTireDamage','ApplyVehicleDamage','GetTireIntegrity','GetSpeedKmh']),
 'roadmap style lock': '<!-- SWIR-ROADMAP-STANDARD:v1 -->' in roadmap and 'DONE-125%2F130' in roadmap and 'ROADMAP-96.2%25' in roadmap,
}
failed=[k for k,v in checks.items() if not v]
if failed: raise SystemExit('Roadblock spike consequence verification failed: '+', '.join(failed))
print(f'Roadblock spike consequence verification passed ({len(checks)} checks).')
