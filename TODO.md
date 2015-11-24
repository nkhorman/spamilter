near term;
	~~ipv6 - spamilter daemon~~ done
	~~ipv6 - ipfwmtad~~ done
	fix / use libspf2
	~~spamilter.rc options validation~~ done
	add config options;
		RBL dns retry
		~~recieved by header checking~~ done
	documentation using man pages;
		add man page for all supporting applcaitons
		elaborate on the usage of each of the applications
		add configuration information for spamilter
		elaboration on what tests are preformed, and their actions, based on config options
	add greylisting globing configuration options
	add support for auto updating the geoip database
	use the new geoip library

long term;
	change build system from home grown to cmake
	ipfwmtad;
		use rule sets for ipfw
		add pf_direct like ipfw_direct

thoughts;
	de-glob prv sender addresses
	add utf-8 subject line handling
	add body handling for;
		uuencoded blobs for DBL scanning
		"I don't care about emails in languages that I can't read"
			Content-Type / charset="" based rejection
	add per-recipient imap based black / white list folder based configuration
	change to a weighting system
		let each test add a measure of weight to a result bucket;
			accept, reject, tempfail, etc...
		and then take action based on weight result of the buckets
		all test weight measusures and result actions must be configurable
	add per-recipient domain/mbox configuration/biasing of test result weights
	ipfwmtad;
		change communiction to use TLS and certificates for authentation
		generalize it so that other applications suites can use it like spamilter does
