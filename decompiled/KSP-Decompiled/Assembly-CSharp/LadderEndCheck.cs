using UnityEngine;

public class LadderEndCheck : MonoBehaviour
{
	private bool reached;

	[SerializeField]
	private KerbalEVA kerbalEVA;

	public bool Reached
	{
		get
		{
			return reached;
		}
		set
		{
			reached = value;
		}
	}

	protected virtual void OnTriggerEnter(Collider o)
	{
		if (o.CompareTag("Ladder"))
		{
			reached = false;
		}
	}

	protected virtual void OnTriggerStay(Collider o)
	{
		if (kerbalEVA.OnALadder && o.CompareTag("Ladder"))
		{
			reached = false;
		}
	}

	protected virtual void OnTriggerExit(Collider o)
	{
		if (o.CompareTag("Ladder"))
		{
			reached = true;
		}
	}
}
