using System;
using KSP.Localization;
using UnityEngine;

[Serializable]
public class DiscoveryInfo : IConfigNode
{
	private IDiscoverable host;

	public KnowledgeItem<string> trackingStatus;

	public KnowledgeItem<UntrackedObjectClass> size;

	public KnowledgeItem<double> signalStrengthPercent;

	public KnowledgeItem<string> signalStrengthLevel;

	public KnowledgeItem<string> name;

	public KnowledgeItem<string> displayName;

	public KnowledgeItem<string> situation;

	public KnowledgeItem<double> distance;

	public KnowledgeItem<double> speed;

	public KnowledgeItem<string> type;

	public KnowledgeItem<float> mass;

	private static string cacheAutoLOC_475347;

	private static string cacheAutoLOC_6002221;

	private static string cacheAutoLOC_6002222;

	private static string cacheAutoLOC_900650;

	private static string cacheAutoLOC_461382;

	private static string cacheAutoLOC_900652;

	private static string cacheAutoLOC_6002223;

	private static string cacheAutoLOC_6002220;

	private static string cacheAutoLOC_6002224;

	private static string cacheAutoLOC_463448;

	private static string cacheAutoLOC_6002225;

	private static string cacheAutoLOC_900381;

	private static string cacheAutoLOC_6002226;

	private static string cacheAutoLOC_7001415;

	private static string cacheAutoLOC_286029;

	private static string cacheAutoLOC_6002227;

	private static string cacheAutoLOC_7001411;

	private static string cacheAutoLOC_6002219;

	private static string cacheAutoLOC_6002228;

	private static string cacheAutoLOC_900380;

	private static string cacheAutoLOC_6002229;

	private static string cacheAutoLOC_7001407;

	private static string cacheAutoLOC_184746;

	private static string cacheAutoLOC_184750;

	private static string cacheAutoLOC_184754;

	private static string cacheAutoLOC_184758;

	private static string cacheAutoLOC_184762;

	private static string cacheAutoLOC_184772;

	private static string cacheAutoLOC_184774;

	private static string cacheAutoLOC_184776;

	private static string cacheAutoLOC_184778;

	private static string cacheAutoLOC_184780;

	private static string cacheAutoLOC_184782;

	private static string cacheAutoLOC_184791;

	private static string cacheAutoLOC_184793;

	private static string cacheAutoLOC_184795;

	private static string cacheAutoLOC_184797;

	private static string cacheAutoLOC_184799;

	private static string cacheAutoLOC_6011124;

	private static string cacheAutoLOC_6011125;

	private static string cacheAutoLOC_6011126;

	private static string cacheAutoLOC_6011129;

	private static string cacheAutoLOC_6002410;

	private static string cacheAutoLOC_6011130;

	private static string cacheAutoLOC_6011131;

	private static string cacheAutoLOC_6011132;

	private static string cacheAutoLOC_6011133;

	public DiscoveryLevels Level { get; private set; }

	public double lastObservedTime { get; private set; }

	public double fadeUT { get; private set; }

	public double unobservedLifetime { get; private set; }

	public double referenceLifetime { get; private set; }

	public UntrackedObjectClass objectSize { get; private set; }

	public DiscoveryInfo(IDiscoverable host)
	{
		Level = DiscoveryLevels.Owned;
		unobservedLifetime = double.PositiveInfinity;
		referenceLifetime = double.PositiveInfinity;
		objectSize = UntrackedObjectClass.C;
		this.host = host;
		if (host != null)
		{
			CompileInfoGetters();
		}
	}

	public DiscoveryInfo(IDiscoverable host, double untrackedLifetime)
	{
		Level = DiscoveryLevels.Owned;
		unobservedLifetime = untrackedLifetime;
		referenceLifetime = untrackedLifetime * 2.0;
		objectSize = UntrackedObjectClass.C;
		this.host = host;
		if (host != null)
		{
			CompileInfoGetters();
		}
	}

	public DiscoveryInfo(IDiscoverable host, DiscoveryLevels level, double untrackedLifetime)
	{
		Level = level;
		unobservedLifetime = untrackedLifetime;
		referenceLifetime = untrackedLifetime * 2.0;
		objectSize = UntrackedObjectClass.C;
		this.host = host;
		if (host != null)
		{
			CompileInfoGetters();
		}
	}

	public void SetLevel(DiscoveryLevels level)
	{
		DiscoveryLevels level2 = Level;
		Level = level;
		GameEvents.onKnowledgeChanged.Fire(new GameEvents.HostedFromToAction<IDiscoverable, DiscoveryLevels>(host, level2, Level));
	}

	public void SetLastObservedTime(double UT)
	{
		lastObservedTime = UT;
		fadeUT = UT + unobservedLifetime;
	}

	public void SetUnobservedLifetime(double time)
	{
		unobservedLifetime = time;
		fadeUT = lastObservedTime + time;
	}

	public void SetReferenceLifetime(double time)
	{
		referenceLifetime = time;
	}

	public void SetUntrackedObjectSize(UntrackedObjectClass size)
	{
		objectSize = size;
	}

	private void CompileInfoGetters()
	{
		trackingStatus = new KnowledgeItem<string>(this, DiscoveryLevels.Presence, cacheAutoLOC_475347 + " ", cacheAutoLOC_6002221, () => (!HaveKnowledgeAbout(DiscoveryLevels.StateVectors)) ? cacheAutoLOC_6002221 : cacheAutoLOC_6002222);
		signalStrengthPercent = new KnowledgeItem<double>(this, DiscoveryLevels.Presence, cacheAutoLOC_900650 + ": ", cacheAutoLOC_461382, () => GetSignalStrength(Planetarium.GetUniversalTime()) * 100.0, "%", "0.#");
		signalStrengthLevel = new KnowledgeItem<string>(this, DiscoveryLevels.Presence, cacheAutoLOC_900650 + ": ", cacheAutoLOC_461382, () => GetSignalStrengthCaption(GetSignalStrength(Planetarium.GetUniversalTime())));
		size = new KnowledgeItem<UntrackedObjectClass>(this, DiscoveryLevels.Presence, cacheAutoLOC_900652 + " ", cacheAutoLOC_6002223, () => objectSize);
		name = new KnowledgeItem<string>(this, DiscoveryLevels.Name, cacheAutoLOC_6002220 + ": ", cacheAutoLOC_6002224, host.RevealName);
		displayName = new KnowledgeItem<string>(this, DiscoveryLevels.Name, cacheAutoLOC_6002220 + ": ", cacheAutoLOC_6002224, host.RevealDisplayName);
		situation = new KnowledgeItem<string>(this, DiscoveryLevels.StateVectors, cacheAutoLOC_463448 + ": ", cacheAutoLOC_6002225, host.RevealSituationString);
		speed = new KnowledgeItem<double>(this, DiscoveryLevels.StateVectors, cacheAutoLOC_900381 + ": ", cacheAutoLOC_6002226, host.RevealSpeed, cacheAutoLOC_7001415, "N0");
		distance = new KnowledgeItem<double>(this, DiscoveryLevels.StateVectors, cacheAutoLOC_286029 + " ", cacheAutoLOC_6002227, host.RevealAltitude, cacheAutoLOC_7001411, "N0");
		type = new KnowledgeItem<string>(this, DiscoveryLevels.Appearance, cacheAutoLOC_6002219 + ": ", cacheAutoLOC_6002228, host.RevealType);
		mass = new KnowledgeItem<float>(this, DiscoveryLevels.Appearance, cacheAutoLOC_900380 + ": ", cacheAutoLOC_6002229, host.RevealMass, cacheAutoLOC_7001407, "0.0##");
	}

	public double GetSignalStrength(double UT)
	{
		return Math.Max(0.0, Math.Min(1.0, unobservedLifetime / referenceLifetime)) * GetSignalLife(UT);
	}

	public double GetSignalLife(double UT)
	{
		if (HaveKnowledgeAbout(DiscoveryLevels.StateVectors))
		{
			return 1.0;
		}
		return Math.Max(0.0, Math.Min(1.0, 1.0 - (UT - lastObservedTime) / (fadeUT - lastObservedTime)));
	}

	public bool HaveKnowledgeAbout(DiscoveryLevels lvl)
	{
		return (lvl & Level) != 0;
	}

	public void Load(ConfigNode node)
	{
		if (node.HasValue("state"))
		{
			Level = (DiscoveryLevels)int.Parse(node.GetValue("state"));
		}
		if (node.HasValue("lastObservedTime"))
		{
			lastObservedTime = double.Parse(node.GetValue("lastObservedTime"));
		}
		if (node.HasValue("lifetime"))
		{
			unobservedLifetime = double.Parse(node.GetValue("lifetime"));
		}
		if (node.HasValue("refTime"))
		{
			referenceLifetime = double.Parse(node.GetValue("refTime"));
		}
		if (node.HasValue("size"))
		{
			objectSize = (UntrackedObjectClass)int.Parse(node.GetValue("size"));
		}
		fadeUT = lastObservedTime + unobservedLifetime;
	}

	public void Save(ConfigNode node)
	{
		node.AddValue("state", (int)Level);
		node.AddValue("lastObservedTime", lastObservedTime);
		node.AddValue("lifetime", unobservedLifetime);
		node.AddValue("refTime", referenceLifetime);
		node.AddValue("size", (int)objectSize);
	}

	public static string GetSignalStrengthCaption(double signal)
	{
		if (signal < 0.2)
		{
			return cacheAutoLOC_184746;
		}
		if (signal < 0.4)
		{
			return cacheAutoLOC_184750;
		}
		if (signal < 0.6)
		{
			return cacheAutoLOC_184754;
		}
		if (signal < 0.8)
		{
			return cacheAutoLOC_184758;
		}
		return cacheAutoLOC_184762;
	}

	public static string GetSizeClassDescription(UntrackedObjectClass sizeClass)
	{
		switch (sizeClass)
		{
		default:
			return cacheAutoLOC_184782;
		case UntrackedObjectClass.A:
			return cacheAutoLOC_184772;
		case UntrackedObjectClass.B:
			return cacheAutoLOC_184774;
		case UntrackedObjectClass.C:
			return cacheAutoLOC_184776;
		case UntrackedObjectClass.D:
			return cacheAutoLOC_184778;
		case UntrackedObjectClass.E:
			return cacheAutoLOC_184780;
		case UntrackedObjectClass.F:
			return cacheAutoLOC_6011130;
		case UntrackedObjectClass.G:
			return cacheAutoLOC_6011131;
		case UntrackedObjectClass.H:
			return cacheAutoLOC_6011132;
		case UntrackedObjectClass.I:
			return cacheAutoLOC_6011133;
		}
	}

	public static string GetSizeClassSizes(UntrackedObjectClass sizeClass)
	{
		switch (sizeClass)
		{
		default:
			return cacheAutoLOC_6002410;
		case UntrackedObjectClass.A:
			return cacheAutoLOC_184791;
		case UntrackedObjectClass.B:
			return cacheAutoLOC_184793;
		case UntrackedObjectClass.C:
			return cacheAutoLOC_184795;
		case UntrackedObjectClass.D:
			return cacheAutoLOC_184797;
		case UntrackedObjectClass.E:
			return cacheAutoLOC_184799;
		case UntrackedObjectClass.F:
			return cacheAutoLOC_6011124;
		case UntrackedObjectClass.G:
			return cacheAutoLOC_6011125;
		case UntrackedObjectClass.H:
			return cacheAutoLOC_6011126;
		case UntrackedObjectClass.I:
			return cacheAutoLOC_6011129;
		}
	}

	public static UntrackedObjectClass GetClassFromSize(float size, int seed)
	{
		UntrackedObjectClass result = UntrackedObjectClass.A;
		foreach (UntrackedObjectClass value in Enum.GetValues(typeof(UntrackedObjectClass)))
		{
			if (GetMinRadius(value, seed) < size && GetMaxRadius(value, seed) > size)
			{
				result = value;
			}
		}
		return result;
	}

	public static float GetClassRadius(UntrackedObjectClass sizeClass, int seed)
	{
		UnityEngine.Random.InitState(seed);
		float num;
		switch (sizeClass)
		{
		default:
			num = 0f;
			break;
		case UntrackedObjectClass.A:
			num = 3.24f;
			break;
		case UntrackedObjectClass.B:
			num = 5.4f;
			break;
		case UntrackedObjectClass.C:
			num = 9f;
			break;
		case UntrackedObjectClass.D:
			num = 15f;
			break;
		case UntrackedObjectClass.E:
			num = 25f;
			break;
		case UntrackedObjectClass.F:
			num = 41.7f;
			break;
		case UntrackedObjectClass.G:
			num = 69.5f;
			break;
		case UntrackedObjectClass.H:
			num = 115.8f;
			break;
		case UntrackedObjectClass.I:
			num = 193f;
			break;
		}
		return num * UnityEngine.Random.Range(0.75f, 1.25f);
	}

	public static UntrackedObjectClass GetObjectClass(string classString)
	{
		UntrackedObjectClass untrackedObjectClass = UntrackedObjectClass.A;
		switch (classString)
		{
		case "D":
			return UntrackedObjectClass.D;
		case "E":
			return UntrackedObjectClass.E;
		case "F":
			return UntrackedObjectClass.F;
		case "G":
			return UntrackedObjectClass.G;
		case "C":
			return UntrackedObjectClass.C;
		case "A":
			return UntrackedObjectClass.A;
		case "H":
			return UntrackedObjectClass.H;
		case "I":
			return UntrackedObjectClass.I;
		default:
			return UntrackedObjectClass.A;
		case "B":
			return UntrackedObjectClass.B;
		}
	}

	public static float GetMinRadius(UntrackedObjectClass sizeClass, int seed)
	{
		return GetClassRadius(sizeClass, seed) * 0.75f;
	}

	public static float GetMaxRadius(UntrackedObjectClass sizeClass, int seed)
	{
		return GetClassRadius(sizeClass, seed) * 1.25f;
	}

	internal static void CacheLocalStrings()
	{
		cacheAutoLOC_475347 = Localizer.Format("#autoLOC_475347");
		cacheAutoLOC_6002221 = Localizer.Format("#autoLOC_6002221");
		cacheAutoLOC_6002222 = Localizer.Format("#autoLOC_6002222");
		cacheAutoLOC_900650 = Localizer.Format("#autoLOC_900650");
		cacheAutoLOC_461382 = Localizer.Format("#autoLOC_461382");
		cacheAutoLOC_900652 = Localizer.Format("#autoLOC_900652");
		cacheAutoLOC_6002223 = Localizer.Format("#autoLOC_6002223");
		cacheAutoLOC_6002220 = Localizer.Format("#autoLOC_6002220");
		cacheAutoLOC_6002224 = Localizer.Format("#autoLOC_6002224");
		cacheAutoLOC_463448 = Localizer.Format("#autoLOC_463448");
		cacheAutoLOC_6002225 = Localizer.Format("#autoLOC_6002225");
		cacheAutoLOC_900381 = Localizer.Format("#autoLOC_900381");
		cacheAutoLOC_6002226 = Localizer.Format("#autoLOC_6002226");
		cacheAutoLOC_7001415 = Localizer.Format("#autoLOC_7001415");
		cacheAutoLOC_286029 = Localizer.Format("#autoLOC_286029");
		cacheAutoLOC_6002227 = Localizer.Format("#autoLOC_6002227");
		cacheAutoLOC_7001411 = Localizer.Format("#autoLOC_7001411");
		cacheAutoLOC_6002219 = Localizer.Format("#autoLOC_6002219");
		cacheAutoLOC_6002228 = Localizer.Format("#autoLOC_6002228");
		cacheAutoLOC_900380 = Localizer.Format("#autoLOC_900380");
		cacheAutoLOC_6002229 = Localizer.Format("#autoLOC_6002229");
		cacheAutoLOC_7001407 = Localizer.Format("#autoLOC_7001407");
		cacheAutoLOC_184746 = Localizer.Format("#autoLOC_184746");
		cacheAutoLOC_184750 = Localizer.Format("#autoLOC_184750");
		cacheAutoLOC_184754 = Localizer.Format("#autoLOC_184754");
		cacheAutoLOC_184758 = Localizer.Format("#autoLOC_184758");
		cacheAutoLOC_184762 = Localizer.Format("#autoLOC_184762");
		cacheAutoLOC_184772 = Localizer.Format("#autoLOC_184772");
		cacheAutoLOC_184774 = Localizer.Format("#autoLOC_184774");
		cacheAutoLOC_184776 = Localizer.Format("#autoLOC_184776");
		cacheAutoLOC_184778 = Localizer.Format("#autoLOC_184778");
		cacheAutoLOC_184780 = Localizer.Format("#autoLOC_184780");
		cacheAutoLOC_184782 = Localizer.Format("#autoLOC_184782");
		cacheAutoLOC_184791 = Localizer.Format("#autoLOC_184791");
		cacheAutoLOC_184793 = Localizer.Format("#autoLOC_184793");
		cacheAutoLOC_184795 = Localizer.Format("#autoLOC_184795");
		cacheAutoLOC_184797 = Localizer.Format("#autoLOC_184797");
		cacheAutoLOC_184799 = Localizer.Format("#autoLOC_184799");
		cacheAutoLOC_6011124 = Localizer.Format("#autoLOC_6011124");
		cacheAutoLOC_6011125 = Localizer.Format("#autoLOC_6011125");
		cacheAutoLOC_6011126 = Localizer.Format("#autoLOC_6011126");
		cacheAutoLOC_6011129 = Localizer.Format("#autoLOC_6011129");
		cacheAutoLOC_6002410 = Localizer.Format("#autoLOC_6002410");
		cacheAutoLOC_6011130 = Localizer.Format("#autoLOC_6011130");
		cacheAutoLOC_6011131 = Localizer.Format("#autoLOC_6011131");
		cacheAutoLOC_6011132 = Localizer.Format("#autoLOC_6011132");
		cacheAutoLOC_6011133 = Localizer.Format("#autoLOC_6011133");
	}
}
