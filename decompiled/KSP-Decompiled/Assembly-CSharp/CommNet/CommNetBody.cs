using CommNet.Occluders;
using UnityEngine;

namespace CommNet
{
	public class CommNetBody : MonoBehaviour
	{
		protected CelestialBody body;

		protected Occluder occluder;

		protected virtual void Start()
		{
			body = GetComponent<CelestialBody>();
			if (CommNetNetwork.Initialized)
			{
				OnNetworkInitialized();
			}
			GameEvents.CommNet.OnNetworkInitialized.Add(OnNetworkInitialized);
		}

		protected virtual void OnDestroy()
		{
			GameEvents.CommNet.OnNetworkInitialized.Remove(OnNetworkInitialized);
		}

		protected virtual void OnNetworkInitialized()
		{
			occluder = CreateOccluder();
			CommNetNetwork.Add(occluder);
		}

		public virtual void OnNetworkPreUpdate()
		{
			occluder.position = body.position;
		}

		protected virtual Occluder CreateOccluder()
		{
			double num = body.Radius * body.scaledRadiusHorizonMultiplier * ((HighLogic.CurrentGame != null) ? ((double)(body.atmosphere ? HighLogic.CurrentGame.Parameters.CustomParams<CommNetParams>().occlusionMultiplierAtm : HighLogic.CurrentGame.Parameters.CustomParams<CommNetParams>().occlusionMultiplierVac)) : 1.0);
			return new OccluderHorizonCulling(body.transform, body.scaledElipRadMult.x * num, body.scaledElipRadMult.y * num, body.scaledElipRadMult.z * num);
		}
	}
}
