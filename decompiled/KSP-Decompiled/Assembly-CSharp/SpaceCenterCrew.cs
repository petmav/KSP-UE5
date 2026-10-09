using UnityEngine;

public class SpaceCenterCrew : MonoBehaviour
{
	public enum CrewType
	{
		GroundCrew,
		Mechanic,
		Scientist
	}

	public enum crewStates
	{
		Idle,
		Walking,
		Running,
		Standing
	}

	public float speed;

	public Transform[] waypoints;

	public int currentwaypoint;

	protected Vector3 target;

	protected Vector3 moveDirection;

	protected Vector3 velocity;

	protected CrewType cType;

	protected crewStates state;

	protected bool stateChanged;

	protected string[] crewAnimations;

	protected Animation _animation;

	protected Rigidbody _rigidbody;

	protected void CrewMovement()
	{
		if (currentwaypoint >= waypoints.Length)
		{
			return;
		}
		target = waypoints[currentwaypoint].position;
		moveDirection = target - base.transform.position;
		velocity = this.GetComponentCached(ref _rigidbody).velocity;
		if (moveDirection.magnitude < 1f)
		{
			this.GetComponentCached(ref _rigidbody).velocity = Vector3.zero;
			if (cType == CrewType.Scientist)
			{
				base.gameObject.transform.LookAt(Vector3.zero);
			}
			currentwaypoint = Random.Range(0, waypoints.Length);
			state = crewStates.Standing;
			stateChanged = true;
		}
		else
		{
			velocity = moveDirection.normalized * speed;
			this.GetComponentCached(ref _rigidbody).velocity = velocity;
			base.transform.LookAt(target);
		}
	}

	protected virtual void SetAnimation()
	{
	}
}
