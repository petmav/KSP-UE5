using UnityEngine;

namespace CommNet
{
	public class CommNetNode : MonoBehaviour
	{
		protected CommNode comm;

		protected bool networkInitialised;

		public CommNode Comm
		{
			get
			{
				return comm;
			}
			set
			{
				comm = value;
			}
		}

		protected virtual void Start()
		{
			comm = new CommNode(base.transform);
			comm.OnNetworkPreUpdate = OnNetworkPreUpdate;
			comm.OnNetworkPostUpdate = OnNetworkPostUpdate;
			if (CommNetNetwork.Initialized)
			{
				networkInitialised = true;
				CommNetNetwork.Add(comm);
			}
			else
			{
				networkInitialised = false;
			}
			GameEvents.CommNet.OnNetworkInitialized.Add(OnNetworkInitialized);
		}

		protected virtual void OnDestroy()
		{
			if (networkInitialised)
			{
				networkInitialised = false;
				CommNetNetwork.Remove(comm);
			}
			GameEvents.CommNet.OnNetworkInitialized.Remove(OnNetworkInitialized);
		}

		protected virtual void OnNetworkInitialized()
		{
			networkInitialised = true;
			CommNetNetwork.Add(comm);
		}

		public virtual void OnNetworkPreUpdate()
		{
		}

		public virtual void OnNetworkPostUpdate()
		{
		}
	}
}
