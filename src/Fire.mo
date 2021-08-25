block Cell "Simulate the behaviour of the cell in the center of the other 8
cell"
	import K = Modelica.Constants;
	import SI = Modelica.SIunits;

	parameter Boolean initialState = false;
	parameter SI.Mass initialFuel = 10;
	parameter SI.Time tau = 1;
	parameter Real theta = 0.2;
	parameter SI.MassFlowRate beta = 0.5; // TODO: questa deve essere derivata
	parameter Real k0 = 1;
	parameter Real k1 = 1;
	parameter Real k2 = 1;
	parameter SI.Length L = 1 "lenght of the side of the cell";
	// NOTE: the number in the center is ignored because of how the offset are
	// used in the loop
	parameter SI.Mass[3,3] gamma = {
		{1, 1, 1},
		{1, 1, 1},
		{1, 1, 1}
	} "converted fuel quantity";
	parameter SI.Height[3,3] P = {
		{1, 1, 1},
		{1, 1, 1},
		{1, 1, 1}
	} "medium height";
	parameter Real[3,3] S = {
		{70, 70, 70},
		{70, 70, 70},
		{70, 70, 70}
	} "inflammability percentage";

	Boolean N "state";
	SI.Mass B "fuel";

	input Boolean[8] Nij "state of an adjacent cells";
	input SI.Mass[3, 3] Bij "fuel of an adjacent cells";
	input SI.Velocity[3,3] F "wind speed";
	input SI.Angle[3,3] D "wind direction";
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
	SI.AngularVelocity fw "wind cotribution";
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
	import Modelica.Blocks.Noise.NormalNoise;
	import Modelica.Blocks.Noise.GlobalSeed;
	Cell c;
	NormalNoise n1(samplePeriod=0.1, sigma=5, mu=0);
	NormalNoise n2(samplePeriod=0.1, sigma=0.4, mu=0);
	inner GlobalSeed globalSeed annotation(HideResult=true);
equation
	c.u = false;
	c.Nij = {true, true, false, false, false, false, false, false};
	c.Bij = {{0.5, 0.5, 0.5}, {0.5, 0.5, 0.5}, {0.5, 0.5, 0.5}};
	c.F = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}} + fill(n2.y, 3, 3);
	c.D = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}} + fill(n1.y, 3, 3);
end Fire;
