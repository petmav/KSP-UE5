using UnityEngine;

public class KSPActionParam
{
	private float _cooldown;

	public KSPActionGroup group { get; private set; }

	public KSPActionType type { get; private set; }

	public float Cooldown
	{
		get
		{
			return _cooldown;
		}
		set
		{
			_cooldown = Mathf.Max(_cooldown, value);
		}
	}

	public KSPActionParam(KSPActionGroup actionGroup, KSPActionType actionType)
	{
		group = actionGroup;
		type = actionType;
		_cooldown = 0f;
	}
}
