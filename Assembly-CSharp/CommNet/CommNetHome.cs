using UnityEngine;

namespace CommNet
{
	public class CommNetHome : MonoBehaviour
	{
		public string nodeName = string.Empty;

		public string displaynodeName = string.Empty;

		public Transform nodeTransform;

		public bool isKSC;

		public bool isPermanent;

		public double antennaPower = 500000.0;

		protected CommNode comm;

		protected CelestialBody body;

		protected double lat;

		protected double lon;

		protected double alt;

		protected virtual void Start()
		{
			body = GetComponentInParent<CelestialBody>();
			if (nodeTransform == null)
			{
				nodeTransform = base.transform;
			}
			if (CommNetNetwork.Initialized)
			{
				OnNetworkInitialized();
			}
			GameEvents.CommNet.OnNetworkInitialized.Add(OnNetworkInitialized);
		}

		protected virtual void OnDestroy()
		{
			if (comm != null)
			{
				CommNetNetwork.Remove(comm);
			}
			GameEvents.CommNet.OnNetworkInitialized.Remove(OnNetworkInitialized);
		}

		protected virtual void Update()
		{
			if (comm != null)
			{
				comm.position = base.transform.position;
			}
		}

		protected virtual void OnNetworkPreUpdate()
		{
			comm.precisePosition = body.GetWorldSurfacePosition(lat, lon, alt);
		}

		protected virtual void OnNetworkInitialized()
		{
			CreateNode();
			if (comm != null)
			{
				CommNetNetwork.Add(comm);
			}
		}

		protected virtual void CreateNode()
		{
			if (HighLogic.CurrentGame != null && !HighLogic.CurrentGame.Parameters.CustomParams<CommNetParams>().enableGroundStations && ((!isKSC && !isPermanent) || (!isKSC && isPermanent && !HighLogic.CurrentGame.Parameters.Difficulty.AllowOtherLaunchSites)))
			{
				if (comm != null)
				{
					CommNetNetwork.Remove(comm);
					comm = null;
				}
				return;
			}
			if (comm == null)
			{
				comm = new CommNode(nodeTransform);
				comm.OnNetworkPreUpdate = OnNetworkPreUpdate;
				comm.isHome = true;
				comm.isControlSource = true;
				comm.isControlSourceMultiHop = true;
			}
			comm.name = nodeName;
			comm.displayName = displaynodeName;
			comm.antennaRelay.Update(isPermanent ? antennaPower : GameVariables.Instance.GetDSNRange(ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.TrackingStation)), GameVariables.Instance.GetDSNRangeCurve(), combine: false);
			if (nodeTransform == null)
			{
				body.GetLatLonAlt(base.transform.position, out lat, out lon, out alt);
			}
			else
			{
				body.GetLatLonAlt(nodeTransform.position, out lat, out lon, out alt);
			}
		}
	}
}
