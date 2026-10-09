using UnityEngine;

namespace Expansions.Serenity
{
	public class RoboticCollisions : MonoBehaviour
	{
		public Part part;

		private void Start()
		{
			if (!ExpansionsLoader.IsExpansionInstalled("Serenity") && HighLogic.LoadedSceneIsGame)
			{
				base.enabled = false;
				Object.Destroy(this);
			}
		}

		private void OnCollisionEnter(Collision other)
		{
			if (part != null)
			{
				part.OnCollisionEnter(other);
			}
		}

		private void OnCollisionExit(Collision other)
		{
			if (part != null)
			{
				part.OnCollisionExit(other);
			}
		}

		private void OnTriggerEnter(Collider other)
		{
			if (part != null)
			{
				part.OnTriggerEnter(other);
			}
		}

		private void OnTriggerExit(Collider other)
		{
			if (part != null)
			{
				part.OnTriggerExit(other);
			}
		}
	}
}
