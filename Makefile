all:
	@$(MAKE) -C controller
	@$(MAKE) -C utilities
	@$(MAKE) -C model_build

clean:
	@$(MAKE) -C controller clean
	@$(MAKE) -C utilities clean
	@$(MAKE) -C model_build clean
