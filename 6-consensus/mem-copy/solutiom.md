# Memory-to-Memory Copy

����� � ������ n �������. �������� ��� ������� one � zero ���� n. ��� ������ Decide ������� #i ����� ������ ���������:
1. Mem-Copy(one[i], zero[i])
2. �������� �� ������� one � ���������� ����� ���������� ������ � ������ �������.  ��� ����� �� ��������� ����������� �������� � ������ zero � ��������� zero ��� ����������� (��� �������� ��������� �������� `agreement`).
3. ������, ��� ��� ��������� � zero ������ �� ����������, �������� �� ������� � ������� ����������� ����� � �������, ��������������� ������ ���������� �������� (��� �������� ��������� ���� �� �������, ������� �������� `validity` �����������). ����� ����� ����� �������, ������ ��� ��� ������� ����� i ��������� ��������.

��� �������� �������� �� ����������� ����� � ������������� ��� ����������� �� ���������� ���������� ������� ������, ������� �������� `wait-free termination` ���� ���������.