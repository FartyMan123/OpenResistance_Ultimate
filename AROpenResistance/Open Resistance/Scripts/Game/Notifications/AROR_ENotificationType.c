//! Kinds of server-sent notification (see ARCHITECTURE.md -> Notifications).
//! Both share one on-screen queue; headlines jump ahead of queued notifications.
enum AROR_ENotificationType
{
	HEADLINE,		//!< Full-width strip across the upper third, for major campaign events
	NOTIFICATION	//!< Dispatch card in the top right, for routine campaign events
}
