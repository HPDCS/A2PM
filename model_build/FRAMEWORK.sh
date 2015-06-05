#!/bin/bash
# Alessandro Pellegrini <pellegrini@dis.uniroma1.i>
# Pierangelo Di Sanzo <disanzo@dis.uniroma1.it>

# Framework Configuration
database_file=db.txt
lambdas=(1  10  100  1000  10000  100000  1000000 10000000 100000000 1000000000)
WEKA_PATH="c:/Programmi/Weka-3-7/weka.jar"
# Valid values for the algorithm are: 
#   grafting
#   iteratedRidge
#   nonNegativeSquared
#   shooting
lasso_algorithm="iteratedRidge"

# What do we have to run?
purge_old_data=false
run_datapoint_aggregation=false
run_lasso=false
apply_lasso=false
plot_original_parameters=false
plot_lasso_as_predictor=false
run_weka_linear=false
run_weka_m5p=false
run_weka_REPtree=false
run_weka_svm=false
run_weka_svm2=false
evaluate_error=true
generate_report=true

# This is a convenience step, not required for the actual learning
# It simply replots everything, in case the plt scripts are modified
replot_models=false






##### DO NOT MODIFY BELOW THIS LINE ####


# Preliminary setup steps

if [ "$run_datapoint_aggregation" = true ] ; then
	rm -rf csv
	rm -rf data
	rm -rf error
	rm -rf gnuplot
	rm latex/*.gen.tex
fi

mkdir -p csv
mkdir -p data
mkdir -p error


if [ "$run_datapoint_aggregation" = true ] ; then
	echo "***************************"
	echo "***  AGGREGATING DATA   ***"
	echo "***************************"
	./aggregate.exe $database_file
fi



if [ "$run_lasso" = true ] ; then
	echo "***************************"
	echo "*** LASSO LINEARIZATION ***"
	echo "***************************"
	
	echo "" > beta-vectors.txt
	
	for lambda in "${lambdas[@]}"; do
		echo Generating beta vector for $lambda;
		matlab -sd . -nodesktop -nosplash -wait -r "lasso('$lasso_algorithm', $lambda)"
	done
fi
	
	
if [ "$apply_lasso" = true ] ; then
	echo "***************************"
	echo "***   APPLYING WEIGHTS  ***"
	echo "***************************"
	
	cp beta-vectors.txt working
	
	while [[ -s working ]] ; do
		lambda_val=$(head working -n 8 | tail -n 5 | grep lambda_val | sed -e 's/.*=//g')
		beta_vector=$(head working -n 8 | tail -n 5 | grep beta | sed -e 's/.*=//g')

		./applyBeta.py $lambda_val $beta_vector
		
		tail -n +8 working > wo
		mv wo working
	done
	
	unlink working
fi



if [ "$plot_original_parameters" = true ] ; then
	echo "***************************"
	echo "*** PLOTTING PARAMETERS ***"
	echo "***************************"
	
	mkdir -p gnuplot/parameters
	
	for lambda in "${lambdas[@]}"; do
	
		echo "Plotting parameters for lambda $lambda"
		
		gnuplot -e "the_title='lasso-lambda-$lambda'" plot_parameters.plt
	done
	
	gnuplot plot_all_parameters.plt
fi




if [ "$plot_lasso_as_predictor" = true ] ; then
	echo "***************************"
	echo "***   PLOTTING LASSO    ***"
	echo "***************************"
	
	mkdir -p gnuplot/lasso
	
	for lambda in "${lambdas[@]}"; do
	
		echo "Plotting Lasso as a predictor for lambda $lambda"
		
		gnuplot -e "the_title='lasso-lambda-$lambda'" plotLasso.plt
	done
fi




if [ "$run_weka_linear" = true ] ; then

	echo "***************************"
	echo "***       LINEAR        ***"
	echo "***************************"

	mkdir -p gnuplot/linear

	echo "Computing error for linear"
	
	java -classpath "$WEKA_PATH" weka.classifiers.functions.LinearRegression -t aggregated.csv -S 0 -R 1.0E-8 -c first -d model > error/linear-error.txt
	
	echo "Generating datapoints for linear"

	java -classpath "$WEKA_PATH"  weka.classifiers.functions.LinearRegression -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > dat.dat
	sed -e '1,5d' dat.dat > data.dat
	unlink dat.dat
	
	unlink model
	
	echo "Generating plot for linear"
	
	gnuplot -e "the_title='linear/linear'" plot.plt
	
	mv data.dat data/linear.dat
fi





if [ "$run_weka_m5p" = true ] ; then
    
	echo "***************************"
	echo "***         M5P         ***"
	echo "***************************"

	mkdir -p gnuplot/m5p

	echo "Computing error for M5P"
	
	java -classpath "$WEKA_PATH"  weka.classifiers.trees.M5P -t aggregated.csv -M 4.0 -c first -d model > error/m5p-error.txt
	
	echo "Generating datapoints for M5P"

	java -classpath "$WEKA_PATH"  weka.classifiers.trees.M5P -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > dat.dat
	sed -e '1,5d' dat.dat > data.dat
	unlink dat.dat
	
	unlink model

	echo "Generating plot for M5P"

	gnuplot -e "the_title='m5p/m5p'" plot.plt

	mv data.dat data/m5p.dat

fi


if [ "$run_weka_REPtree" = true ] ; then
    
	echo "***************************"
	echo "***       REP Tree      ***"
	echo "***************************"

	mkdir -p gnuplot/reptree

	echo "Computing error for REP Tree"
	
	java -classpath "$WEKA_PATH"  weka.classifiers.trees.REPTree -t aggregated.csv -M 2 -V 0.001 -N 3 -S 1 -L -1 -c first -d model > error/REPTree-error.txt
	
	echo "Generating datapoints for REP Tree"

	java -classpath "$WEKA_PATH"  weka.classifiers.trees.REPTree -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > dat.dat
	sed -e '1,5d' dat.dat > data.dat
	unlink dat.dat
	
	unlink model

	echo "Generating plot for REP Tree"

	gnuplot -e "the_title='reptree/reptree'" plot.plt

	mv data.dat data/reptree.dat

fi



if [ "$run_weka_svm" = true ] ; then
	echo "***************************"
	echo "***        SVM          ***"
	echo "***************************"

	mkdir -p gnuplot/svm

	echo "Computing error for SVM"
	
	java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -t aggregated.csv -C 1.0 -N 0 -I "weka.classifiers.functions.supportVector.RegSMOImproved -T 0.001 -V -P 1.0E-12 -L 0.001 -W 1" -K "weka.classifiers.functions.supportVector.PolyKernel -E 1.0 -C 250007" -c first -d model > error/svm-error.txt 
	
	echo "Generating datapoints for SVM"

	java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > dat.dat
	sed -e '1,5d' dat.dat > data.dat
	unlink dat.dat
	
	unlink model

	echo "Generating plot for SVM"

	gnuplot -e "the_title='svm/svm'" plot.plt

	mv data.dat data/svm.dat
fi



if [ "$run_weka_svm2" = true ] ; then
	echo "***************************"
	echo "***       SVM 2         ***"
	echo "***************************"

	mkdir -p gnuplot/svm2

	echo "Computing error for SVM2"
			
	java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -t aggregated.csv -C 1.0 -N 0 -I "weka.classifiers.functions.supportVector.RegSMOImproved -T 0.001 -P 1.0E-12 -L 0.001 -W 1" -K "weka.classifiers.functions.supportVector.PolyKernel -E 1.0 -C 250007" -c first -d model > error/svm2-error.txt 
	
	echo "Generating datapoints for SVM2"

	java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > dat.dat
	sed -e '1,5d' dat.dat > data.dat
	unlink dat.dat
	
	unlink model

	echo "Generating plot for SVM2"

	gnuplot -e "the_title='svm2/svm2'" plot.plt

	mv data.dat data/svm2.dat
fi


if [ "$evaluate_error" = true ] ; then
	echo "***************************"
	echo "***       ERROR         ***"
	echo "***************************"
	
	./buildErrorTables.py
	mv *.gen.tex latex
	
	echo "All errors computed"
	
fi



if [ "$generate_report" = true ] ; then
	echo "***************************"
	echo "***  GENERATING REPORT  ***"
	echo "***************************"
	
	echo "" > latex/gen_time.gen.tex
	echo "" > latex/val_time.gen.tex
	
	echo "Extracting Generation and Validation Time"
	for file in error/*.txt; do
		title=$(basename $file)
		title=${title%.*}
		title=$(echo $title | sed 's/-error//g')
		gen_time=$(cat $file | grep "Time taken to build model" | sed 's/.*: //' | sed 's/ seconds//')
		val_time=$(cat $file | grep "Time taken to test model on training data" | sed 's/.*: //' | sed 's/ seconds//')
		echo -e "$title\t&\t$gen_time\\\\\\" >> latex/gen_time.gen.tex
		echo -e "$title\t&\t$val_time\\\\\\" >> latex/val_time.gen.tex
	done
	
	echo "Extracting Lasso Parameters"
	echo "" > latex/parameters.gen.tex
	echo "" > latex/num_parameters.gen.tex
	echo "" > LassoParameters.dat
	count=0
	header_files=( csv/* )
	grep beta beta-vectors.txt | while IFS= read -r line
	do
		head_count=0
		line=$(echo ${line:6:${#line}} | sed 's/\]//')
		line=( ${line//,/ } )
		header_string=$(head -n 1 ${header_files[$count]} | sed 's/.*"RTTC",//' | sed 's/_/\\_/g' | sed 's/\"//g' )
		lambda=$(basename ${header_files[$count]})
		lambda=${lambda%.*}
		lambda=$(echo $lambda | sed 's/lasso-lambda-//g')
		lambda_full=$lambda
		lambda=$(echo "l($lambda)/l(10)" | bc -l | sed 's/\..*//')
		
		echo "Getting non-zero parameters for lambda 10^$lambda..."
		
		# Generate latex header for table
		echo "\\paragraph{\$\\lambda = 10^{$lambda}\$}" >> latex/parameters.gen.tex
		echo "\begin{center} \begin{tabular}{cc} \toprule parameter & weight \\\\ \midrule" >> latex/parameters.gen.tex
		
		# Do actual matching
		headers=( ${header_string//,/ } )
		for parameter in "${line[@]}"; do
			if [ $(echo " $parameter != 0" | bc) -eq 1 ]; then
				echo -e "\\\\tt ${headers[$head_count]}\t&\t$parameter\\\\\\" >> latex/parameters.gen.tex
				let head_count=head_count+1
			fi
		done
		echo -e "\$\\lambda = 10^{$lambda}\$ \t&\t $head_count \\\\\\" >> latex/num_parameters.gen.tex
		echo -e "$lambda_full \t $head_count" >> LassoParameters.dat
		let count=count+1
		
		# Generate latex footer for table
		echo "\bottomrule" >> latex/parameters.gen.tex
		echo "\end{tabular} \end{center}" >> latex/parameters.gen.tex
		echo "" >> latex/parameters.gen.tex
	done
	
	echo "Building Report..."
	
	gnuplot LassoParameters.plt
	unlink LassoParameters.dat
	
	today=$(date "+%Y-%m-%d")
	
	cd latex; pdflatex report.tex; mv report.pdf report-$today.pdf; rm *.log; rm *.aux; unlink report.pdf
	
	echo "Report built in latex/report-$today.pdf"
fi





if [ "$replot_models" = true ] ; then
	echo "***************************"
	echo "***  REPLOTTING MODELS  ***"
	echo "***************************"
	
	for f in data/*.dat
	do
		name=$(echo $f | sed -e 's/.*\///')
		name=$(echo $name | sed -e 's/-/\//')
		name=$(echo $name | sed -e 's/\.dat//')
	
		cp $f data.dat
		
		echo "Generating plot for $name"
		gnuplot -e "the_title='$name'" plot.plt
		
	done
	
	unlink data.dat
	
fi
