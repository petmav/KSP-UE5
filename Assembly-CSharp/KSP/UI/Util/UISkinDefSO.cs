using UnityEngine;

namespace KSP.UI.Util
{
	public class UISkinDefSO : ScriptableObject
	{
		[SerializeField]
		private UISkinDef skinDef;

		public UISkinDef SkinDef => skinDef;
	}
}
