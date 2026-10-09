using TMPro;

namespace KSP.UI.Screens
{
	public class EditorActionGroup_Base : UISelectableGridLayoutGroupItem
	{
		protected EditorActionGroupType _type;

		public TextMeshProUGUI groupName;

		public EditorActionGroupType type
		{
			get
			{
				return _type;
			}
			private set
			{
				_type = value;
			}
		}
	}
}
