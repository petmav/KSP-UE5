using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RUIHoverController
{
	private struct CallbackID
	{
		public int id;

		public Callback callback;

		public CallbackID(int id, Callback callback)
		{
			this.id = id;
			this.callback = callback;
		}
	}

	private bool refreshRequested1;

	private bool refreshRequested2;

	public float refreshTimeToWait = 0.5f;

	private bool updateDaemonRunning;

	private MonoBehaviour host;

	private Queue<CallbackID> schedule = new Queue<CallbackID>();

	private HashSet<int> scheduleIds = new HashSet<int>();

	private Dictionary<int, Callback> scheduleOut = new Dictionary<int, Callback>();

	private int waitForHowerOutId = -1;

	public RUIHoverController(MonoBehaviour host)
	{
		this.host = host;
		host.StartCoroutine(UpdateDaemon());
	}

	public void Schedule(int id, Callback callback)
	{
		if (schedule.Count <= 1)
		{
			if (!scheduleIds.Contains(id))
			{
				schedule.Enqueue(new CallbackID(id, callback));
				scheduleIds.Add(id);
				refreshRequested1 = true;
			}
			else if (scheduleOut.ContainsKey(id))
			{
				scheduleOut.Remove(id);
			}
		}
	}

	public void Deschedule(int id, Callback callback)
	{
		if (scheduleIds.Contains(id) && !scheduleOut.ContainsKey(id))
		{
			scheduleOut.Add(id, callback);
			refreshRequested2 = true;
		}
	}

	private IEnumerator UpdateDaemon()
	{
		yield return null;
		yield return null;
		if (updateDaemonRunning)
		{
			yield break;
		}
		updateDaemonRunning = true;
		while ((bool)host)
		{
			if (refreshRequested1 && waitForHowerOutId == -1 && schedule.Count > 0)
			{
				schedule.Peek().callback();
				waitForHowerOutId = schedule.Peek().id;
				refreshRequested1 = false;
				refreshRequested2 = true;
				yield return null;
			}
			if (refreshRequested2 && scheduleOut.ContainsKey(waitForHowerOutId))
			{
				CallbackID callbackID = schedule.Dequeue();
				scheduleIds.Remove(callbackID.id);
				scheduleOut[waitForHowerOutId]();
				scheduleOut.Remove(waitForHowerOutId);
				waitForHowerOutId = -1;
				refreshRequested1 = true;
				refreshRequested2 = false;
			}
			yield return null;
		}
	}
}
