using UnityEngine;

[RequireComponent(typeof(Rigidbody))]
public class PQS_ColliderRigidbodyTest : MonoBehaviour
{
	public float force;

	public PQS rootSphere;

	public float sphereGravity;

	private Vector3 initPos;

	private Quaternion initRot;

	private Rigidbody _rigidbody;

	private float fps;

	private void Reset()
	{
		force = 100f;
		sphereGravity = 0f - (float)PhysicsGlobals.GravitationalAcceleration;
	}

	private void Start()
	{
		this.GetComponentCached(ref _rigidbody).useGravity = false;
		initPos = base.transform.position;
		initRot = base.transform.rotation;
	}

	private void FixedUpdate()
	{
		if (Input.GetMouseButton(0))
		{
			Vector3 normalized = (base.transform.position - Camera.main.transform.position).normalized;
			this.GetComponentCached(ref _rigidbody).AddForce(normalized * force);
		}
		if (Input.GetMouseButton(2))
		{
			this.GetComponentCached(ref _rigidbody).velocity = Vector3.zero;
			this.GetComponentCached(ref _rigidbody).angularVelocity = Vector3.zero;
		}
		if (Input.GetKeyDown(KeyCode.R))
		{
			base.transform.position = initPos;
			base.transform.rotation = initRot;
			this.GetComponentCached(ref _rigidbody).velocity = Vector3.zero;
			this.GetComponentCached(ref _rigidbody).angularVelocity = Vector3.zero;
		}
		this.GetComponentCached(ref _rigidbody).AddForce(sphereGravity * (base.transform.position - rootSphere.transform.position).normalized);
	}

	private void Update()
	{
		fps = Mathf.Lerp(fps, 1f / Time.deltaTime, 10f * Time.deltaTime);
	}
}
