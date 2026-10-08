using System;
using System.Collections.Generic;
using System.IO;
using Expansions.Missions.Adjusters;
using Expansions.Missions.Editor;
using Expansions.Missions.Flow;
using Expansions.Missions.Runtime;
using KSP.Localization;
using Steamworks;
using UnityEngine;

namespace Expansions.Missions
{
	public static class MissionsUtils
	{
		internal class Objectives
		{
			internal class Graph<T>
			{
				public Dictionary<MENode, HashSet<MENode>> AdjacencyList { get; private set; }

				public Graph()
				{
					AdjacencyList = new Dictionary<MENode, HashSet<MENode>>();
				}

				public Graph(IEnumerable<MENode> vertices, IEnumerable<MEEdge> edges)
					: this()
				{
					foreach (MENode vertex in vertices)
					{
						AddVertex(vertex);
					}
					foreach (MEEdge edge in edges)
					{
						AddEdge(edge);
					}
				}

				public void AddVertex(MENode vertex)
				{
					AdjacencyList[vertex] = new HashSet<MENode>();
				}

				public void AddEdge(MEEdge edge)
				{
					if (AdjacencyList.ContainsKey(edge.from) && AdjacencyList.ContainsKey(edge.to))
					{
						AdjacencyList[edge.from].Add(edge.to);
						AdjacencyList[edge.from].Add(edge.to);
					}
				}
			}

			internal class MEEdge
			{
				public MENode from;

				public MENode to;
			}

			public static HashSet<MENode> BreadthSearch<T>(Graph<MENode> graph, MENode start, Action<MENode> preVisit = null)
			{
				HashSet<MENode> hashSet = new HashSet<MENode>();
				if (!graph.AdjacencyList.ContainsKey(start))
				{
					return hashSet;
				}
				Queue<MENode> queue = new Queue<MENode>();
				queue.Enqueue(start);
				while (queue.Count > 0)
				{
					MENode mENode = queue.Dequeue();
					if (hashSet.Contains(mENode))
					{
						continue;
					}
					preVisit?.Invoke(mENode);
					hashSet.Add(mENode);
					foreach (MENode item in graph.AdjacencyList[mENode])
					{
						if (!hashSet.Contains(item))
						{
							queue.Enqueue(item);
						}
					}
				}
				return hashSet;
			}
		}

		internal static readonly List<string> ImageExtensions = new List<string> { ".jpg", ".jpe", ".bmp", ".tga", ".png" };

		internal static readonly string savesPath = "missions/";

		internal static readonly string testsavesPath = "test_missions/";

		public static readonly string fileExtension = ".mission";

		private static Callback<SteamUGCQueryCompleted_t, bool, bool> onSteamQueryComplete;

		private static DictionaryValueList<string, MissionPack> missionPacks;

		public static DictionaryValueList<string, List<Type>> adjusterTypesSupportedByPartModule = new DictionaryValueList<string, List<Type>>();

		public static DictionaryValueList<string, List<Type>> partModuleTypesSupportedByAdjuster = new DictionaryValueList<string, List<Type>>();

		public static string ExpansionRootPath => KSPExpansionsUtils.ExpansionsGameDataPath + "MakingHistory/";

		public static string StockMissionsPath => ExpansionRootPath + "Missions/";

		public static string BaseMissionsPath => KSPUtil.ApplicationRootPath + "GameData/Squad/Missions/";

		public static string UsersMissionsPath => KSPUtil.ApplicationRootPath + "Missions/";

		public static string MissionExportsPath => UsersMissionsPath + "_Exports/";

		public static string MissionImportCompletedPath => UsersMissionsPath + "_Imported/";

		public static string TemplatesPath => ExpansionRootPath + "Templates/";

		public static string SavesRootPath => KSPUtil.ApplicationRootPath + "saves/" + SavesPath;

		public static string SavesPath
		{
			get
			{
				if (!MissionSystem.IsTestMode && HighLogic.LoadedScene != GameScenes.MISSIONBUILDER)
				{
					return savesPath;
				}
				return testsavesPath;
			}
		}

		public static DictionaryValueList<string, MissionPack> MissionPacks
		{
			get
			{
				if (missionPacks == null)
				{
					RebuildPacksList();
				}
				return missionPacks;
			}
		}

		public static void OpenMissionBuilder()
		{
			if (!GameSettings.TUTORIALS_MISSION_BUILDER_ENTERED)
			{
				GameSettings.TUTORIALS_MISSION_BUILDER_ENTERED = true;
				GameSettings.SaveSettings();
			}
			BundleLoader.LoadScene(GameScenes.MISSIONBUILDER, "makinghistory_scene", "Assets/Expansions/Missions/_Scenes/kspMissionEditor.unity");
		}

		public static GameObject MEPrefab(string prefabPath)
		{
			GameObject gameObject = (GameObject)BundleLoader.LoadAsset("makinghistory_assets", "Assets/Expansions/Missions/" + prefabPath);
			if (gameObject != null)
			{
				return gameObject;
			}
			int length = prefabPath.LastIndexOf(".prefab", StringComparison.InvariantCulture);
			prefabPath = prefabPath.Substring(0, length);
			int num = prefabPath.LastIndexOf('/') + 1;
			prefabPath = prefabPath.Substring(num, prefabPath.Length - num);
			return AssetBase.GetPrefab(prefabPath);
		}

		public static Texture2D METexture(string prefabPath)
		{
			return BundleLoader.LoadTextureAsset("makinghistory_assets", "Assets/Expansions/Missions/" + prefabPath);
		}

		public static int GetFacilityLimit(string facilityName, int defaultLevel)
		{
			switch (facilityName)
			{
			case "ResearchAndDevelopment":
				return HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsFacilities>().facilityLevelRD;
			case "AstronautComplex":
				return HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsFacilities>().facilityLevelAC;
			case "Runway":
				return HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsFacilities>().facilityLevelRunway;
			case "LaunchPad":
				return HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsFacilities>().facilityLevelLaunchpad;
			case "Administration":
				return HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsFacilities>().facilityLevelAdmin;
			case "TrackingStation":
				return HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsFacilities>().facilityLevelTS;
			case "SpaceplaneHangar":
				return HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsFacilities>().facilityLevelSPH;
			case "MissionControl":
				return HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsFacilities>().facilityLevelMC;
			default:
				return defaultLevel;
			case "VehicleAssemblyBuilding":
				return HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsFacilities>().facilityLevelVAB;
			}
		}

		public static string GetMissionsFolder(MissionTypes missionType)
		{
			switch (missionType)
			{
			default:
				return StockMissionsPath;
			case MissionTypes.User:
				return UsersMissionsPath;
			case MissionTypes.Steam:
				return SteamManager.KSPSteamWorkshopFolder;
			case MissionTypes.Base:
				return BaseMissionsPath;
			}
		}

		public static string GetUsersMissionsPath(string missionName)
		{
			if (string.IsNullOrEmpty(missionName))
			{
				return "";
			}
			return string.Concat(UsersMissionsPath + KSPUtil.SanitizeFilename(missionName), "/persistent.mission");
		}

		public static string GetStockMissionsPath(string missionName)
		{
			if (string.IsNullOrEmpty(missionName))
			{
				return "";
			}
			return string.Concat(StockMissionsPath + KSPUtil.SanitizeFilename(missionName), "/persistent.mission");
		}

		public static string GetBaseMissionsPath(string missionName)
		{
			if (string.IsNullOrEmpty(missionName))
			{
				return "";
			}
			return string.Concat(BaseMissionsPath + KSPUtil.SanitizeFilename(missionName), "/persistent.mission");
		}

		internal static List<MissionFileInfo> GatherMissionFiles(MissionTypes missionType, bool excludeSteamUnsubscribed = false)
		{
			List<MissionFileInfo> list = new List<MissionFileInfo>();
			if (missionType == MissionTypes.Steam && !SteamManager.Initialized)
			{
				return list;
			}
			string missionsFolder = GetMissionsFolder(missionType);
			if (Directory.Exists(missionsFolder))
			{
				DirectoryInfo[] directories = new DirectoryInfo(missionsFolder).GetDirectories();
				int num = directories.Length;
				for (int i = 0; i < num; i++)
				{
					string name = directories[i].Name;
					if (File.Exists(missionsFolder + name + "/persistent.mission"))
					{
						MissionFileInfo missionFileInfo = null;
						try
						{
							missionFileInfo = new MissionFileInfo(name, missionType);
						}
						catch (Exception ex)
						{
							Debug.LogErrorFormat("[MissionUtils] Error loading mission: {0}/persistent.mission - {1}", missionsFolder + name, ex.Message);
						}
						if (missionFileInfo != null && (!(missionFileInfo.missionType == MissionTypes.Steam && excludeSteamUnsubscribed) || missionFileInfo.subscribed))
						{
							list.Add(missionFileInfo);
						}
					}
				}
			}
			Localizer.AppendMissionTags(Localizer.CurrentLanguage);
			try
			{
				list.Sort(SortMissions);
			}
			catch (Exception)
			{
			}
			return list;
		}

		internal static int SortMissions(MissionFileInfo a, MissionFileInfo b)
		{
			if (a.order == b.order)
			{
				return a.title.CompareTo(b.title);
			}
			return a.order.CompareTo(b.order);
		}

		public static string VerifyUsersMissionsFolder(string missionName)
		{
			if (!Directory.Exists(UsersMissionsPath))
			{
				Directory.CreateDirectory(UsersMissionsPath);
			}
			string text = UsersMissionsPath + KSPUtil.SanitizeFilename(missionName);
			if (!Directory.Exists(text))
			{
				Directory.CreateDirectory(text);
			}
			if (!Directory.Exists(text + "/Ships"))
			{
				Directory.CreateDirectory(text + "/Ships");
			}
			if (!Directory.Exists(text + "/Ships/VAB"))
			{
				Directory.CreateDirectory(text + "/Ships/VAB");
			}
			if (!Directory.Exists(text + "/Ships/SPH"))
			{
				Directory.CreateDirectory(text + "/Ships/SPH");
			}
			if (!Directory.Exists(text + "/Banners"))
			{
				Directory.CreateDirectory(text + "/Banners");
			}
			MEBannerType[] array = (MEBannerType[])Enum.GetValues(typeof(MEBannerType));
			for (int i = 0; i < array.Length; i++)
			{
				if (!Directory.Exists(text + "/Banners/" + array[i]))
				{
					Directory.CreateDirectory(text + "/Banners/" + array[i]);
				}
			}
			return text;
		}

		public static void RemoveMissionSaves(string folderName)
		{
			string path = KSPUtil.ApplicationRootPath + "saves/" + savesPath + folderName + "/";
			if (Directory.Exists(path))
			{
				Directory.Delete(path, recursive: true);
			}
			path = KSPUtil.ApplicationRootPath + "saves/" + testsavesPath + folderName + "/";
			if (Directory.Exists(path))
			{
				Directory.Delete(path, recursive: true);
			}
		}

		internal static void GetSteamWorkshopMissions(Callback<SteamUGCQueryCompleted_t, bool, bool> Callback, EUGCQuery queryType, string[] tags)
		{
			onSteamQueryComplete = Callback;
			SteamManager.Instance.QueryUGCItems(onSteamQueryItemsCallback, queryType, EUGCMatchingUGCType.k_EUGCMatchingUGCType_Items_ReadyToUse, tags, 1u, returnMetadata: true);
		}

		internal static void onSteamQueryItemsCallback(SteamUGCQueryCompleted_t results, bool bIOFailure)
		{
			bool arg = false;
			if (!bIOFailure && results.m_eResult == EResult.k_EResultOK && 1 < SteamManager.Instance.CurrentUGCQueryTotalPages)
			{
				arg = true;
			}
			if (onSteamQueryComplete != null)
			{
				onSteamQueryComplete(results, bIOFailure, arg);
			}
			else
			{
				SteamUGC.ReleaseQueryUGCRequest(results.m_handle);
			}
		}

		public static void RebuildPacksList()
		{
			if (missionPacks == null)
			{
				missionPacks = new DictionaryValueList<string, MissionPack>();
			}
			ConfigNode[] configNodes = GameDatabase.Instance.GetConfigNodes("MISSIONPACK");
			for (int i = 0; i < configNodes.Length; i++)
			{
				AddMissionPackFromNode(configNodes[i]);
			}
			ParseMissionPacksCFG(UsersMissionsPath);
		}

		internal static void ParseMissionPacksCFG(string folderPath)
		{
			if (missionPacks == null)
			{
				RebuildPacksList();
			}
			string[] array = new string[0];
			if (Directory.Exists(folderPath))
			{
				array = Directory.GetFiles(folderPath, "MissionPacks.cfg", SearchOption.AllDirectories);
			}
			for (int i = 0; i < array.Length; i++)
			{
				ConfigNode[] nodes = ConfigNode.Load(array[i]).GetNodes("MISSIONPACK");
				for (int j = 0; j < nodes.Length; j++)
				{
					AddMissionPackFromNode(nodes[j]);
				}
			}
		}

		private static void AddMissionPackFromNode(ConfigNode node)
		{
			MissionPack missionPack = new MissionPack();
			missionPack.Load(node);
			if (!missionPacks.ContainsKey(missionPack.name))
			{
				missionPacks.Add(missionPack.name, missionPack);
			}
		}

		public static List<MissionPack> GatherMissionPacks(MissionTypes missionType)
		{
			List<MissionPack> list = new List<MissionPack>();
			for (int i = 0; i < MissionPacks.ValuesList.Count; i++)
			{
				MissionPack missionPack = MissionPacks.ValuesList[i];
				if (missionType == MissionTypes.Stock && missionPack.name.StartsWith("squad_"))
				{
					list.Add(missionPack);
				}
				else if ((missionType == MissionTypes.Stock || missionType == MissionTypes.Base) && missionPack.name.StartsWith("squad_ESA"))
				{
					list.Add(missionPack);
				}
				else if ((missionType == MissionTypes.User || missionType == MissionTypes.Steam) && !missionPack.name.StartsWith("squad_"))
				{
					list.Add(missionPack);
				}
			}
			if (missionType != 0 && missionType != MissionTypes.Base)
			{
				list.Sort(MissionPack.CompareDisplayName);
			}
			else
			{
				list.Sort(MissionPack.CompareOrder);
			}
			return list;
		}

		public static void InitialiseAdjusterTypes()
		{
			List<Type> typesOfClassesImplementingInterface = AssemblyLoader.GetTypesOfClassesImplementingInterface<IPartModuleAdjuster>();
			for (int i = 0; i < typesOfClassesImplementingInterface.Count; i++)
			{
				AdjusterPartModuleBase adjusterPartModuleBase = null;
				if (!typesOfClassesImplementingInterface[i].IsAbstract)
				{
					adjusterPartModuleBase = (AdjusterPartModuleBase)Activator.CreateInstance(typesOfClassesImplementingInterface[i], new object[1]);
				}
				if (adjusterPartModuleBase == null)
				{
					continue;
				}
				List<Type> partModulesThatCanBeAffected = adjusterPartModuleBase.GetPartModulesThatCanBeAffected();
				partModuleTypesSupportedByAdjuster[typesOfClassesImplementingInterface[i].Name] = partModulesThatCanBeAffected;
				for (int j = 0; j < partModulesThatCanBeAffected.Count; j++)
				{
					if (!adjusterTypesSupportedByPartModule.ContainsKey(partModulesThatCanBeAffected[j].Name))
					{
						adjusterTypesSupportedByPartModule[partModulesThatCanBeAffected[j].Name] = new List<Type>();
					}
					adjusterTypesSupportedByPartModule[partModulesThatCanBeAffected[j].Name].AddUnique(typesOfClassesImplementingInterface[i]);
				}
			}
		}

		public static void AddMissionsScenarios()
		{
			List<KSPScenarioType> allScenarioTypesInAssemblies = KSPScenarioType.GetAllScenarioTypesInAssemblies();
			List<ProtoScenarioModule> list = new List<ProtoScenarioModule>();
			foreach (KSPScenarioType item in allScenarioTypesInAssemblies)
			{
				if (!item.ScenarioAttributes.HasCreateOption(ScenarioCreationOptions.AddToNewMissionGames) || item.ModuleType.Name == "ResearchAndDevelopment" || item.ModuleType.Name == "ScenarioUpgradeableFacilities")
				{
					continue;
				}
				ConfigNode configNode = new ConfigNode("SCENARIO");
				configNode.AddValue("name", item.ModuleType.Name);
				string text = "";
				bool flag = false;
				int i = 0;
				for (int num = item.ScenarioAttributes.TargetScenes.Length; i < num; i++)
				{
					if (item.ScenarioAttributes.TargetScenes[i] == GameScenes.MISSIONBUILDER)
					{
						flag = true;
					}
					string text2 = text;
					int num2 = (int)item.ScenarioAttributes.TargetScenes[i];
					text = text2 + num2;
					if (i < item.ScenarioAttributes.TargetScenes.Length - 1)
					{
						text += ", ";
					}
				}
				if (!flag)
				{
					text += ((item.ScenarioAttributes.TargetScenes.Length != 0) ? ", " : "");
					text += 21;
				}
				configNode.AddValue("scene", text);
				ProtoScenarioModule protoScenarioModule = new ProtoScenarioModule(configNode);
				if (protoScenarioModule != null)
				{
					list.Add(protoScenarioModule);
				}
			}
			ScenarioRunner.SetProtoModules(list);
		}

		internal static List<MENode> GetNodesOrdered(Mission m, bool objectivesOnly)
		{
			List<Objectives.MEEdge> list = new List<Objectives.MEEdge>();
			foreach (MENode values in m.nodes.ValuesList)
			{
				foreach (MENode fromNode in values.fromNodes)
				{
					list.Add(new Objectives.MEEdge
					{
						from = fromNode,
						to = values
					});
				}
			}
			HashSet<MENode> hashSet = Objectives.BreadthSearch<MENode>(new Objectives.Graph<MENode>(m.nodes.ValuesList, list), m.startNode);
			List<MENode> list2 = new List<MENode>();
			foreach (MENode item in hashSet)
			{
				if (!objectivesOnly || item.isObjective)
				{
					list2.Add(item);
				}
			}
			return list2;
		}

		public static MEFlowParser InstantiateMEFlowParser(Transform parent)
		{
			GameObject gameObject = new GameObject("MEFlowParser");
			gameObject.transform.SetParent(parent);
			return gameObject.gameObject.AddComponent<MEFlowParser>();
		}

		public static Texture2D GetTextureInExternalPath(string filePath)
		{
			Texture2D texture2D = null;
			if (File.Exists(filePath) && ImageExtensions.Contains(Path.GetExtension(filePath).ToLowerInvariant()))
			{
				byte[] data = File.ReadAllBytes(filePath);
				texture2D = new Texture2D(2, 2, TextureFormat.RGBA32, mipChain: false);
				texture2D.LoadImage(data);
			}
			return texture2D;
		}

		public static string GetFieldValueForDisplay(BaseAPField field)
		{
			if (field.FieldType.BaseType == typeof(Enum))
			{
				return (field.GetValue() as Enum).displayDescription();
			}
			if (field.FieldType == typeof(bool))
			{
				if (!(bool)field.GetValue())
				{
					return Localizer.Format("#autoLOC_439840");
				}
				return Localizer.Format("#autoLOC_439839");
			}
			return field.GetValue().ToString();
		}

		internal static void UpdatePartNames(ref List<string> partNames)
		{
			if (partNames != null)
			{
				for (int i = 0; i < partNames.Count; i++)
				{
					partNames[i] = UpdatePartName(partNames[i]);
				}
			}
		}

		internal static string UpdatePartName(string partName)
		{
			switch (partName)
			{
			case "mk1pod":
				return "mk1pod.v2";
			case "probeCoreHex":
				return "probeCoreHex.v2";
			case "probeCoreOcto":
				return "probeCoreOcto.v2";
			case "probeCoreOcto2":
				return "probeCoreOcto2.v2";
			case "probeCoreSphere":
				return "probeCoreSphere.v2";
			case "roverBody":
				return "roverBody.v2";
			case "solidBooster.sm":
				return "solidBooster.sm.v2";
			case "solidBooster":
				return "solidBooster.v2";
			case "mk2LanderCabin":
				return "mk2LanderCabin.v2";
			case "liquidEngineMini":
				return "liquidEngineMini.v2";
			case "liquidEngine2-2":
				return "liquidEngine2-2.v2";
			case "liquidEngine3":
				return "liquidEngine3.v2";
			case "Size3To2Adapter":
				return "Size3To2Adapter.v2";
			case "stackBiCoupler":
				return "stackBiCoupler.v2";
			case "stackTriCoupler":
				return "stackTriCoupler.v2";
			case "smallRadialEngine":
				return "smallRadialEngine.v2";
			case "microEngine":
				return "microEngine.v2";
			case "radialEngineMini":
				return "radialEngineMini.v2";
			case "fuelTank1-2":
				return "Rockomax32.BW";
			case "fuelTank2-2":
				return "Rockomax16.BW";
			case "fuelTank3-2":
				return "Rockomax64.BW";
			case "fuelTank4-2":
				return "Rockomax8BW";
			case "Mark1-2Pod":
				return "mk1-3pod";
			case "stackDecoupler":
				return "Decoupler.1";
			case "decoupler1-2":
				return "Decoupler.2";
			case "stackDecouplerMini":
				return "Decoupler.0";
			case "size3Decoupler":
				return "Decoupler.3";
			case "stackSeparator":
				return "Separator.1";
			case "stackSeparatorBig":
				return "Separator.2";
			case "stackSeparatorMini":
				return "Separator.0";
			case "toroidalFuelTank":
				return "externalTankToroid";
			default:
				return partName;
			}
		}
	}
}
