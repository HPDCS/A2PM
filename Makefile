all:
	@$(MAKE) -C controller
	@$(MAKE) -C feature_collection
	@$(MAKE) -C utilities

clean:
	@$(MAKE) -C controller clean
	@$(MAKE) -C feature_collection clean
	@$(MAKE) -C utilities
