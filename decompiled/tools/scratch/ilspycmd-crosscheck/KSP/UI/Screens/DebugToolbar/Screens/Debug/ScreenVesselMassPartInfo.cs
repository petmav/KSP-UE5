using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.Debug
{
	public class ScreenVesselMassPartInfo : MonoBehaviour
	{
		[SerializeField]
		private GameObject header;

		[SerializeField]
		private TextMeshProUGUI partName;

		[SerializeField]
		private TextMeshProUGUI stage;

		[SerializeField]
		private TextMeshProUGUI prefabMass;

		[SerializeField]
		private TextMeshProUGUI resourceMass;

		[SerializeField]
		private TextMeshProUGUI inventoryMass;

		[SerializeField]
		private TextMeshProUGUI kerbalMass;

		[SerializeField]
		private TextMeshProUGUI kerbalResourceMass;

		[SerializeField]
		private TextMeshProUGUI kerbalInventoryMass;

		[SerializeField]
		private TextMeshProUGUI totalMass;

		private Part part;

		public void UpdateData(Part part, bool firstRow)
		{
			if (header != null)
			{
				header.SetActive(firstRow);
			}
			this.part = part;
			partName.text = part.partInfo.title;
			stage.text = part.stageOffset.ToString("0");
			prefabMass.text = part.prefabMass.ToString("0.000");
			resourceMass.text = part.resourceMass.ToString("0.000");
			inventoryMass.text = part.inventoryMass.ToString("0.000");
			kerbalMass.text = part.kerbalMass.ToString("0.000");
			kerbalResourceMass.text = part.kerbalResourceMass.ToString("0.000");
			kerbalInventoryMass.text = part.kerbalInventoryMass.ToString("0.000");
			totalMass.text = (part.mass + part.resourceMass).ToString("0.000");
		}
	}
}
