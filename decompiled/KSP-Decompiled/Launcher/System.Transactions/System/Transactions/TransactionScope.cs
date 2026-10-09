namespace System.Transactions;

public sealed class TransactionScope : IDisposable
{
	private static TransactionOptions defaultOptions = new TransactionOptions(IsolationLevel.Serializable, TransactionManager.DefaultTimeout);

	private Transaction transaction;

	private Transaction oldTransaction;

	private TransactionScope parentScope;

	private TimeSpan timeout;

	private int nested;

	private bool disposed;

	private bool completed;

	private bool isRoot;

	private bool asyncFlowEnabled;

	internal bool IsComplete => completed;

	internal TimeSpan Timeout => timeout;

	public TransactionScope()
		: this(TransactionScopeOption.Required, TransactionManager.DefaultTimeout)
	{
	}

	public TransactionScope(TransactionScopeAsyncFlowOption asyncFlowOption)
		: this(TransactionScopeOption.Required, TransactionManager.DefaultTimeout, asyncFlowOption)
	{
	}

	public TransactionScope(Transaction transactionToUse)
		: this(transactionToUse, TransactionManager.DefaultTimeout)
	{
	}

	public TransactionScope(Transaction transactionToUse, TimeSpan scopeTimeout)
		: this(transactionToUse, scopeTimeout, EnterpriseServicesInteropOption.None)
	{
	}

	[System.MonoTODO("EnterpriseServicesInteropOption not supported.")]
	public TransactionScope(Transaction transactionToUse, TimeSpan scopeTimeout, EnterpriseServicesInteropOption interopOption)
	{
		Initialize(TransactionScopeOption.Required, transactionToUse, defaultOptions, interopOption, scopeTimeout, TransactionScopeAsyncFlowOption.Suppress);
	}

	public TransactionScope(TransactionScopeOption scopeOption)
		: this(scopeOption, TransactionManager.DefaultTimeout)
	{
	}

	public TransactionScope(TransactionScopeOption scopeOption, TimeSpan scopeTimeout)
		: this(scopeOption, scopeTimeout, TransactionScopeAsyncFlowOption.Suppress)
	{
	}

	public TransactionScope(TransactionScopeOption option, TransactionScopeAsyncFlowOption asyncFlow)
		: this(option, TransactionManager.DefaultTimeout, asyncFlow)
	{
	}

	public TransactionScope(TransactionScopeOption scopeOption, TimeSpan scopeTimeout, TransactionScopeAsyncFlowOption asyncFlow)
	{
		Initialize(scopeOption, null, defaultOptions, EnterpriseServicesInteropOption.None, scopeTimeout, asyncFlow);
	}

	public TransactionScope(TransactionScopeOption scopeOption, TransactionOptions transactionOptions)
		: this(scopeOption, transactionOptions, EnterpriseServicesInteropOption.None)
	{
	}

	[System.MonoTODO("EnterpriseServicesInteropOption not supported")]
	public TransactionScope(TransactionScopeOption scopeOption, TransactionOptions transactionOptions, EnterpriseServicesInteropOption interopOption)
	{
		Initialize(scopeOption, null, transactionOptions, interopOption, TransactionManager.DefaultTimeout, TransactionScopeAsyncFlowOption.Suppress);
	}

	public TransactionScope(Transaction transactionToUse, TransactionScopeAsyncFlowOption asyncFlowOption)
	{
		throw new NotImplementedException();
	}

	public TransactionScope(Transaction transactionToUse, TimeSpan scopeTimeout, TransactionScopeAsyncFlowOption asyncFlowOption)
	{
		throw new NotImplementedException();
	}

	public TransactionScope(TransactionScopeOption scopeOption, TransactionOptions transactionOptions, TransactionScopeAsyncFlowOption asyncFlowOption)
	{
		throw new NotImplementedException();
	}

	private void Initialize(TransactionScopeOption scopeOption, Transaction tx, TransactionOptions options, EnterpriseServicesInteropOption interop, TimeSpan scopeTimeout, TransactionScopeAsyncFlowOption asyncFlow)
	{
		completed = false;
		isRoot = false;
		nested = 0;
		asyncFlowEnabled = asyncFlow == TransactionScopeAsyncFlowOption.Enabled;
		if (scopeTimeout < TimeSpan.Zero)
		{
			throw new ArgumentOutOfRangeException("scopeTimeout");
		}
		timeout = scopeTimeout;
		oldTransaction = Transaction.CurrentInternal;
		Transaction.CurrentInternal = (transaction = InitTransaction(tx, scopeOption));
		if (transaction != null)
		{
			transaction.InitScope(this);
		}
		if (parentScope != null)
		{
			parentScope.nested++;
		}
	}

	private Transaction InitTransaction(Transaction tx, TransactionScopeOption scopeOption)
	{
		if (tx != null)
		{
			return tx;
		}
		switch (scopeOption)
		{
		case TransactionScopeOption.Suppress:
			if (Transaction.CurrentInternal != null)
			{
				parentScope = Transaction.CurrentInternal.Scope;
			}
			return null;
		case TransactionScopeOption.Required:
			if (Transaction.CurrentInternal == null)
			{
				isRoot = true;
				return new Transaction();
			}
			parentScope = Transaction.CurrentInternal.Scope;
			return Transaction.CurrentInternal;
		default:
			if (Transaction.CurrentInternal != null)
			{
				parentScope = Transaction.CurrentInternal.Scope;
			}
			isRoot = true;
			return new Transaction();
		}
	}

	public void Complete()
	{
		if (completed)
		{
			throw new InvalidOperationException("The current TransactionScope is already complete. You should dispose the TransactionScope.");
		}
		completed = true;
	}

	public void Dispose()
	{
		if (disposed)
		{
			return;
		}
		disposed = true;
		if (parentScope != null)
		{
			parentScope.nested--;
		}
		if (nested > 0)
		{
			transaction.Rollback();
			throw new InvalidOperationException("TransactionScope nested incorrectly");
		}
		if (Transaction.CurrentInternal != transaction && !asyncFlowEnabled)
		{
			if (transaction != null)
			{
				transaction.Rollback();
			}
			if (Transaction.CurrentInternal != null)
			{
				Transaction.CurrentInternal.Rollback();
			}
			throw new InvalidOperationException("Transaction.Current has changed inside of the TransactionScope");
		}
		if (asyncFlowEnabled)
		{
			if (oldTransaction != null)
			{
				oldTransaction.Scope = parentScope;
			}
			Transaction currentInternal = Transaction.CurrentInternal;
			if (!(transaction == null) || !(currentInternal == null))
			{
				currentInternal.Scope = parentScope;
				Transaction.CurrentInternal = oldTransaction;
				transaction.Scope = null;
				if (!IsComplete)
				{
					transaction.Rollback();
					currentInternal.Rollback();
				}
				else if (isRoot)
				{
					currentInternal.CommitInternal();
					transaction.CommitInternal();
				}
			}
			return;
		}
		if (Transaction.CurrentInternal == oldTransaction && oldTransaction != null)
		{
			oldTransaction.Scope = parentScope;
		}
		Transaction.CurrentInternal = oldTransaction;
		if (!(transaction == null))
		{
			transaction.Scope = null;
			if (!IsComplete)
			{
				transaction.Rollback();
			}
			else if (isRoot)
			{
				transaction.CommitInternal();
			}
		}
	}
}
