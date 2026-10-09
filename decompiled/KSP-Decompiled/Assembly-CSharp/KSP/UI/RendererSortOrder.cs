using UnityEngine;

namespace KSP.UI
{
	[RequireComponent(typeof(Renderer))]
	public class RendererSortOrder : MonoBehaviour
	{
		[SerializeField]
		protected string layerName;

		[SerializeField]
		protected int sortingOrder;

		private void Start()
		{
			Set();
		}

		[ContextMenu("Set")]
		public void Set()
		{
			Renderer component = GetComponent<Renderer>();
			component.sortingLayerName = layerName;
			component.sortingOrder = sortingOrder;
		}
	}
}
