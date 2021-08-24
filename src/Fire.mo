block Cell "Simulate the behaviour of the cell in the center of the other 8
cell"
	import K = Modelica.Constants;

	parameter Boolean initialState = false;
	parameter Real initialFuel = 10;
	parameter Real tau = 1;
	parameter Real theta = 0.2;
	parameter Real beta = 0.5;
	parameter Real k0 = 1;
	parameter Real k1 = 1;
	parameter Real k2 = 1;
	parameter Real L = 1 "lenght of the side of the cell";
	// NOTE: the number in the center is ignored because of how the offset are
	// used in the loop
	parameter Real[3,3] gamma = {
		{1, 1, 1},
		{1, 1, 1},
		{1, 1, 1}
	} "fuel quantity",
	D = {
		{1, 1, 1},
		{1, 1, 1},
		{1, 1, 1}
	} "wind direction",
	P = {
		{1, 1, 1},
		{1, 1, 1},
		{1, 1, 1}
	} "medium height",
	F = {
		{1, 1, 1},
		{1, 1, 1},
		{1, 1, 1}
	} "wind speed",
	S = {
		{70, 70, 70},
		{70, 70, 70},
		{70, 70, 70}
	} "inflammability percentage";

	Boolean N "state";
	Real B "fuel";

	input Boolean[8] Nij "state of an adjacent cells";
	input Real[3, 3] Bij "fuel of an adjacent cells";
	input Boolean u "exogenous input";
// protected
	constant Integer Gamma[8, 2] = {
		{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}
	} "directions array";
	constant Integer i = 2 "center abscissa";
	constant Integer j = 2 "center ordinate";

	Real[8] p "fire transmission probability from another cell";
	Real C "Combustion state of the cell";
	Real d "disomogeneity factor of cells border";
	Real fw "wind cotribution";
	Real fP "slope cotribution";
	Integer e1 "x offset", e2 "y offset";
	Boolean V "fire transmission by and adjacent cell";
algorithm
	when initial() then
		N := initialState;
		B := initialFuel;
	elsewhen sample(0, tau) then
		// calculating the fire transmission probability
		for index in 1:8 loop
			e1 := Gamma[index, 1];
			e2 := Gamma[index, 2];
			C := sin(K.pi*Bij[i+e1,j+e2]/gamma[i+e1,j+e2]);
			d := (1 - 0.5 * abs(e1*e2));
			fw := exp(
				k1*F[i+e1,j+e2]*(e1*cos(D[i+e1,j+e2])+e2*sin(D[i+e1,j+e2]))
				/sqrt(e1*e1 + e2*e2)
			);
			fP := exp(k2*atan((P[i,j]-P[i+e1,j+e2])/L));
			p[index] := k0 * S[i,j] * C * d * fw * fP;
		end for;
		// checking if the fire transmission happened
		V := max(p[i]*(if Nij[i] then 1 else 0) > theta for i in 1:8);
		// evolving the model
		N := if pre(B) > 0 then (V or u) else false;
		B := if pre(N) then max(0, pre(B) - beta*tau) else pre(B);
	end when;
end Cell;

model Fire
	Cell centralCell;
equation
	centralCell.u = false;
	centralCell.Nij = {true, true, false, false, false, false, false, false};
	centralCell.Bij = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};
end Fire;
