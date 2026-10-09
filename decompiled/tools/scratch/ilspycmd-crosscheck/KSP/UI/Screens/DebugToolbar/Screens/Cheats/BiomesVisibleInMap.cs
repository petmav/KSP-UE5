using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Cheats
{
	public class BiomesVisibleInMap : DebugScreenToggle
	{
		protected override void SetupValues()
		{
			SetToggle(CheatOptions.BiomesVisible);
		}

		protected override void OnToggleChanged(bool state)
		{
			if (CheatOptions.BiomesVisible != state)
			{
				SetBiomesVisible(state);
			}
		}

		private void SetBiomesVisible(bool isTrue)
		{
			CheatOptions.BiomesVisible = isTrue;
			int i = 0;
			for (int count = FlightGlobals.fetch.bodies.Count; i < count; i++)
			{
				CelestialBody celestialBody = FlightGlobals.fetch.bodies[i];
				GameObject scaledBody = celestialBody.scaledBody;
				if (celestialBody == null || scaledBody == null)
				{
					continue;
				}
				MeshRenderer component = scaledBody.GetComponent<MeshRenderer>();
				if (component.material.HasProperty("_ResourceMap"))
				{
					if (isTrue && celestialBody.BiomeMap != null)
					{
						component.material.SetTexture("_ResourceMap", celestialBody.BiomeMap.CompileToTexture());
					}
					else
					{
						component.material.SetTexture("_ResourceMap", null);
					}
				}
			}
		}
	}
}
