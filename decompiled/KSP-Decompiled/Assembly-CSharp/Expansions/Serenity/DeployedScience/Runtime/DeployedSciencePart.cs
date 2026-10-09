using UnityEngine;

namespace Expansions.Serenity.DeployedScience.Runtime
{
	public class DeployedSciencePart : MonoBehaviour, IConfigNode
	{
		[SerializeField]
		private uint partId;

		[SerializeField]
		private bool deployedOnGround;

		[SerializeField]
		private bool moduleEnabled;

		[SerializeField]
		private int powerUnitsProduced;

		[SerializeField]
		private int actualPowerUnitsProduced;

		[SerializeField]
		private int powerUnitsRequired;

		[SerializeField]
		private bool isSolarPanel;

		[SerializeField]
		private bool isAntenna;

		[SerializeField]
		private double antennaBoosterPower;

		public DeployedScienceExperiment Experiment;

		[SerializeField]
		private uint controllerId;

		public DeployedScienceCluster Cluster;

		private ModuleGroundSciencePart loadedPartModule;

		public uint PartId => partId;

		public bool DeployedOnGround => deployedOnGround;

		public bool Enabled
		{
			get
			{
				return moduleEnabled;
			}
			set
			{
				if (loadedPartModule == null)
				{
					FlightGlobals.FindLoadedPart(partId, out var partout);
					if (partout != null)
					{
						loadedPartModule = partout.FindModuleImplementing<ModuleGroundSciencePart>();
					}
				}
				moduleEnabled = value;
				if (loadedPartModule != null && loadedPartModule.Enabled != moduleEnabled)
				{
					loadedPartModule.Enabled = moduleEnabled;
				}
				else if (Cluster != null)
				{
					Cluster.UpdatePowerState();
				}
			}
		}

		public int PowerUnitsProduced => powerUnitsProduced;

		public int ActualPowerUnitsProduced => actualPowerUnitsProduced;

		public int PowerUnitsRequired => powerUnitsRequired;

		public bool IsSolarPanel => isSolarPanel;

		public bool IsAntenna => isAntenna;

		public double AntennaBoosterPower => antennaBoosterPower;

		public uint ControllerId => controllerId;

		private static DeployedSciencePart Spawn()
		{
			GameObject obj = new GameObject("DeployedSciencePart");
			obj.transform.SetParent(DeployedScience.DeployedScienceGameObject.transform);
			return obj.gameObject.AddComponent<DeployedSciencePart>();
		}

		public static DeployedSciencePart Spawn(ModuleGroundSciencePart part, ModuleGroundExpControl controlUnit, DeployedScienceCluster cluster)
		{
			DeployedSciencePart deployedSciencePart = Spawn();
			if (cluster != null)
			{
				deployedSciencePart.Cluster = cluster;
				deployedSciencePart.gameObject.transform.SetParent(cluster.transform);
			}
			if (controlUnit != null)
			{
				deployedSciencePart.controllerId = controlUnit.part.persistentId;
			}
			if (part != null)
			{
				deployedSciencePart.partId = part.part.persistentId;
				deployedSciencePart.UpdateSciencePart(part);
				deployedSciencePart.gameObject.name = "DeployedSciencePart " + deployedSciencePart.PartId;
			}
			return deployedSciencePart;
		}

		public static DeployedSciencePart SpawnandLoad(ConfigNode node, DeployedScienceCluster cluster)
		{
			DeployedSciencePart deployedSciencePart = Spawn();
			if (cluster != null)
			{
				deployedSciencePart.Cluster = cluster;
				deployedSciencePart.gameObject.transform.SetParent(cluster.transform);
			}
			deployedSciencePart.Load(node);
			if (deployedSciencePart.Experiment != null && cluster != null)
			{
				deployedSciencePart.Experiment.gameObject.transform.SetParent(deployedSciencePart.gameObject.transform);
				deployedSciencePart.Experiment.Cluster = cluster;
				deployedSciencePart.Experiment.sciencePart = deployedSciencePart;
			}
			deployedSciencePart.Cluster = cluster;
			deployedSciencePart.gameObject.name = "DeployedSciencePart " + deployedSciencePart.PartId;
			return deployedSciencePart;
		}

		private void Awake()
		{
			if (!ExpansionsLoader.IsExpansionInstalled("Serenity"))
			{
				Object.Destroy(this);
			}
		}

		private void Start()
		{
			GameEvents.onGroundSciencePartChanged.Add(UpdateSciencePart);
			GameEvents.onGroundSciencePartRemoved.Add(OnGroundSciencePartRemoved);
			GameEvents.onGroundSciencePartEnabledStateChanged.Add(UpdateSciencePart);
		}

		private void OnDestroy()
		{
			GameEvents.onGroundSciencePartChanged.Remove(UpdateSciencePart);
			GameEvents.onGroundSciencePartRemoved.Remove(OnGroundSciencePartRemoved);
			GameEvents.onGroundSciencePartEnabledStateChanged.Remove(UpdateSciencePart);
			if (Experiment != null)
			{
				Experiment.gameObject.DestroyGameObject();
			}
		}

		private void OnGroundSciencePartRemoved(ModuleGroundSciencePart sciencePartRemoved)
		{
			if (ControllerId == sciencePartRemoved.ControlUnitId && sciencePartRemoved.part.persistentId != sciencePartRemoved.ControlUnitId)
			{
				controllerId = 0u;
			}
		}

		public void UpdateSciencePart(ModuleGroundSciencePart part)
		{
			if (PartId != part.part.persistentId)
			{
				return;
			}
			moduleEnabled = part.Enabled;
			deployedOnGround = part.DeployedOnGround;
			powerUnitsProduced = part.PowerUnitsProduced;
			actualPowerUnitsProduced = part.ActualPowerUnitsProduced;
			powerUnitsRequired = part.PowerUnitsRequired;
			isSolarPanel = part.IsSolarPanel;
			ModuleGroundExperiment component = part.gameObject.GetComponent<ModuleGroundExperiment>();
			if (component != null)
			{
				if (Experiment == null)
				{
					if (Cluster != null)
					{
						Experiment = DeployedScienceExperiment.Spawn(component, Cluster);
						Experiment.gameObject.transform.SetParent(base.transform);
						Experiment.sciencePart = this;
					}
				}
				else
				{
					Experiment.UpdateScienceExperiment(component);
				}
			}
			isAntenna = false;
			antennaBoosterPower = 0.0;
			ModuleGroundCommsPart component2 = part.gameObject.GetComponent<ModuleGroundCommsPart>();
			if (component2 != null)
			{
				isAntenna = true;
				antennaBoosterPower = component2.antennaPower;
			}
			if (Cluster != null)
			{
				if (isSolarPanel)
				{
					Cluster.solarPanelParts.AddUnique(this);
				}
				if (isAntenna)
				{
					Cluster.antennaParts.AddUnique(this);
				}
				Cluster.UpdatePowerState();
			}
		}

		public Part PartIsLoaded()
		{
			Part partout = null;
			if (partId != 0)
			{
				FlightGlobals.FindLoadedPart(partId, out partout);
				return partout;
			}
			return partout;
		}

		public void Load(ConfigNode node)
		{
			node.TryGetValue("PartId", ref partId);
			node.TryGetValue("Enabled", ref moduleEnabled);
			node.TryGetValue("DeployedOnGround", ref deployedOnGround);
			node.TryGetValue("PowerUnitsProduced", ref powerUnitsProduced);
			node.TryGetValue("ActualPowerUnitsProduced", ref actualPowerUnitsProduced);
			node.TryGetValue("PowerUnitsRequired", ref powerUnitsRequired);
			node.TryGetValue("IsSolarPanel", ref isSolarPanel);
			node.TryGetValue("IsAntenna", ref isAntenna);
			node.TryGetValue("AntennaBoosterPower", ref antennaBoosterPower);
			ConfigNode node2 = new ConfigNode();
			if (node.TryGetNode("EXPERIMENT", ref node2))
			{
				Experiment = DeployedScienceExperiment.SpawnandLoad(node2);
			}
		}

		public void Save(ConfigNode node)
		{
			node.AddValue("PartId", PartId);
			node.AddValue("Enabled", Enabled);
			node.AddValue("DeployedOnGround", DeployedOnGround);
			node.AddValue("PowerUnitsProduced", PowerUnitsProduced);
			node.AddValue("ActualPowerUnitsProduced", actualPowerUnitsProduced);
			node.AddValue("PowerUnitsRequired", PowerUnitsRequired);
			node.AddValue("IsSolarPanel", IsSolarPanel);
			node.AddValue("IsAntenna", IsAntenna);
			node.AddValue("AntennaBoosterPower", AntennaBoosterPower);
			if (Experiment != null)
			{
				ConfigNode node2 = node.AddNode("EXPERIMENT");
				Experiment.Save(node2);
			}
		}
	}
}
