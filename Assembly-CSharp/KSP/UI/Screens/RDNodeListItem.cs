using TMPro;
using UnityEngine;

namespace KSP.UI.Screens
{
	[RequireComponent(typeof(UIListItem))]
	public class RDNodeListItem : MonoBehaviour
	{
		public RDNode node;

		public TextMeshProUGUI text;
	}
}
