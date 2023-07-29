#
# duktape-cc tutorial markdown script run.
#
# djs has to be in your execution PATH.
#
DJS=djs
MDFILE=overview.md
EXAMPLE=
ARGS=arg1 "ARG 2" "argument 3" 4 5 6

.PHONY: clean run html

clean:
	@rm -rf ./tmp

run:
	@mkdir -p ./tmp
	@echo "Running with EXAMPLE='$(EXAMPLE)', ARGS='$(ARGS)'"
	@cd ./tmp && echo "STDIN-INPUT" | $(DJS) -s ../run-example.djs "../$(MDFILE)" "$(EXAMPLE)" $(ARGS)
	@rm -rf ./tmp

html: ./tmp/$(patsubst %.md,%.html,$(MDFILE))

tmp/%.html: %.md
	@mkdir -p ./tmp
	@md2html $< > $@
