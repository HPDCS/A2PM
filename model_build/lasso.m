function lasso(algorithm, LAMBDA)

X=dlmread('aggregated.csv',',',10,0);
Y=X(:,1);
X=X(:,[2:size(X,2)]);
size(Y);
size(X);

setpath

if strcmp(algorithm, 'grafting') == 1
	weights = LassoGrafting(X, Y, LAMBDA);
elseif strcmp(algorithm, 'iteratedRidge') == 1
	weights = LassoIteratedRidge(X, Y, LAMBDA);
elseif strcmp(algorithm, 'nonNegativeSquared') == 1
	weights = LassoNonNegativeSquared(X, Y, LAMBDA);
elseif strcmp(algorithm, 'shooting') == 1
	weights = LassoShooting(X, Y, LAMBDA);
else
	errordlg('Invalid Lasso Algorithm selected')
	quit
end

weights_str = sprintf('%.15f,', weights);
weights_str = weights_str(1:end-1);% strip final comma

% Save output on text file, for later processing
diary beta-vectors.txt
diary on

disp(sprintf('\n\n***\n\nlambda_val=%s\n\nbeta=[%s]', num2str(LAMBDA), weights_str));

diary off

quit

end