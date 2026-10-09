using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens
{
	public class DebugScreenInputDouble : MonoBehaviour
	{
		public TextMeshProUGUI labelText;

		public TMP_InputField inputField;

		public string label = "";

		public double defaultValue;

		private double value;

		public double Value
		{
			get
			{
				return value;
			}
			set
			{
				this.value = value;
				inputField.text = value.ToString();
			}
		}

		private void Awake()
		{
			inputField.onValueChanged.AddListener(OnInputChanged);
		}

		private void Start()
		{
			inputField.text = value.ToString();
		}

		protected virtual void OnInputChanged(string state)
		{
			double result = 0.0;
			if (double.TryParse(state, out result))
			{
				value = result;
			}
		}
	}
}
