//! Shared entry check. The server rechecks every debug request; never log credentials.
class AROR_MarsAccess
{
	static bool Accepts(string credential)
	{
		return credential == "Vote4Chicken";
	}
}
