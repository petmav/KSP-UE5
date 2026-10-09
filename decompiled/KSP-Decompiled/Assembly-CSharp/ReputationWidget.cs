using UnityEngine;

public class ReputationWidget : CurrencyWidget
{
	public Gauge gauge;

	private void Awake()
	{
		GameEvents.OnReputationChanged.Add(onReputationChanged);
	}

	private void OnDestroy()
	{
		GameEvents.OnReputationChanged.Remove(onReputationChanged);
	}

	private void onReputationChanged(float rep, TransactionReasons reason)
	{
		gauge.setValue(Mathf.InverseLerp(0f - Reputation.RepRange, Reputation.RepRange, rep) * 2f - 1f);
	}

	public override void DelayedStart()
	{
		if (Reputation.Instance != null)
		{
			onReputationChanged(Reputation.Instance.reputation, TransactionReasons.None);
		}
	}

	public override bool OnAboutToStart()
	{
		if (HighLogic.CurrentGame != null)
		{
			return Reputation.Instance != null;
		}
		return false;
	}
}
