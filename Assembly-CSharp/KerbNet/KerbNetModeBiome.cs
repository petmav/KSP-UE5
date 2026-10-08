using System;
using KSP.Localization;
using UnityEngine;

namespace KerbNet
{
	internal class KerbNetModeBiome : KerbNetMode
	{
		private CelestialBody bodyCache;

		private OrbitDriver driverCache;

		private bool sunCache;

		private CBAttributeMapSO biomeCache;

		private static string cacheAutoLOC_438890;

		private static string cacheAutoLOC_258912;

		public override void OnInit()
		{
			name = "Biome";
			displayName = cacheAutoLOC_438890;
			buttonSprite = Resources.Load<Sprite>("Scanners/biome");
			localCoordinateInfoLabel = cacheAutoLOC_438890;
			doCoordinatePass = true;
			doTerrainContourPass = true;
			doAnomaliesPass = true;
		}

		public override void OnPrecache(Vessel vessel)
		{
			bodyCache = vessel.mainBody;
			driverCache = ((bodyCache != null) ? bodyCache.orbitDriver : null);
			sunCache = bodyCache != null && bodyCache == Planetarium.fetch.Sun;
			biomeCache = ((bodyCache != null) ? bodyCache.BiomeMap : null);
		}

		public override Color GetCoordinateColor(Vessel vessel, double currentLatitude, double currentLongitude)
		{
			currentLatitude *= Math.PI / 180.0;
			currentLongitude *= Math.PI / 180.0;
			CBAttributeMapSO.MapAttribute mapAttribute = ((biomeCache != null) ? biomeCache.GetAtt(currentLatitude, currentLongitude) : null);
			if (mapAttribute != null)
			{
				return mapAttribute.mapColor;
			}
			if (driverCache != null)
			{
				KerbNetMode.hsv.FromColor(driverCache.Renderer.orbitColor);
				KerbNetMode.hsv.v = KerbNetMode.hsv.v + (UnityEngine.Random.value * 0.4f - 0.2f);
				return KerbNetMode.hsv.ToColor();
			}
			if (sunCache)
			{
				return Color.Lerp(Color.yellow, Color.white, UnityEngine.Random.value);
			}
			return Color.black;
		}

		public override string LocalCoordinateInfo(Vessel vessel, double centerLatitude, double centerLongitude, double waypointLatitude, double waypointLongitude, bool waypointInSpace)
		{
			if (waypointInSpace)
			{
				return cacheAutoLOC_258912;
			}
			CelestialBody mainBody = vessel.mainBody;
			CBAttributeMapSO.MapAttribute biome = ResourceUtilities.GetBiome(ResourceUtilities.Deg2Rad(centerLatitude), ResourceUtilities.Deg2Rad(centerLongitude), mainBody);
			string text = ((biome != null) ? Localizer.Format("#autoLOC_7001301", biome.displayname) : Localizer.Format("#autoLOC_438936", mainBody.displayName));
			biome = ResourceUtilities.GetBiome(ResourceUtilities.Deg2Rad(waypointLatitude), ResourceUtilities.Deg2Rad(waypointLongitude), mainBody);
			return (biome != null) ? Localizer.Format("#autoLOC_7001301", biome.displayname) : text;
		}

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_438890 = Localizer.Format("#autoLOC_438890");
			cacheAutoLOC_258912 = Localizer.Format("#autoLOC_258912").ToUpper();
		}
	}
}
