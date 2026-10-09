using UnityEngine;

namespace KSP.UI
{
	public abstract class UICameraBase : MonoBehaviour
	{
		[SerializeField]
		protected Camera cam;

		private void Reset()
		{
			cam = GetComponent<Camera>();
		}
	}
}
