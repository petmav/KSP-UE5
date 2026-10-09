using TMPro;

public class ScienceWidget : CurrencyWidget
{
	public TextMeshProUGUI text;

	private void Awake()
	{
		GameEvents.OnScienceChanged.Add(onScienceChange);
	}

	private void OnDestroy()
	{
		GameEvents.OnScienceChanged.Remove(onScienceChange);
	}

	private void onScienceChange(float sci, TransactionReasons reason)
	{
		text.text = KSPUtil.LocalizeNumber(sci, "0.0");
	}

	public override void DelayedStart()
	{
		if (ResearchAndDevelopment.Instance != null)
		{
			onScienceChange(ResearchAndDevelopment.Instance.Science, TransactionReasons.None);
		}
	}

	public override bool OnAboutToStart()
	{
		if (HighLogic.CurrentGame != null)
		{
			return ResearchAndDevelopment.Instance != null;
		}
		return false;
	}
}
