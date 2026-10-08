using UnityEngine;

namespace EdyCommonTools
{
	public class AttachToTarget : MonoBehaviour
	{
		public Transform target;

		private void LateUpdate()
		{
			if ((bool)target)
			{
				base.transform.position = target.position;
				base.transform.rotation = target.rotation;
			}
		}
	}
}
