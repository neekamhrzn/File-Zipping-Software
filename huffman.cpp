#include<iostream>
#include<SFML/Graphics.hpp>
#include<string>
#include "huffman.hpp"

void huffman::createArr() {
    for (int i = 0; i < 128; i++) {
        arr.push_back(new Node());
        arr[i]->data = i;
        arr[i]->freq = 0;
    }
}

void huffman::traverse(Node* r, string str) {
    if (r->left == NULL && r->right == NULL) {
        r->code = str;
        return;
    }

    traverse(r->left, str + '0');
    traverse(r->right, str + '1');
}

int huffman::binToDec(string inStr) {
    int res = 0;
    for (auto c : inStr) {
        res = res * 2 + c - '0';
    }
    return res;
}

string huffman::decToBin(int inNum) {
    string temp = "", res = "";
    while (inNum > 0) {
        temp += (inNum % 2 + '0');
        inNum /= 2;
    }
    res.append(8 - temp.length(), '0');
    for (int i = temp.length() - 1; i >= 0; i--) {
        res += temp[i];
    }
    return res;
}

void huffman::buildTree(char a_code, string& path) {
    Node* curr = root;
    for (int i = 0; i < int(path.length()); i++) {
        if (path[i] == '0') {
            if (curr->left == NULL) {
                curr->left = new Node();
            }
            curr = curr->left;
        }
        else if (path[i] == '1') {
            if (curr->right == NULL) {
                curr->right = new Node();
            }
            curr = curr->right;
        }
    }
    curr->data = a_code;
}

void huffman::createMinHeap() {
    char id;
    inFile.open(inFileName, ios::in);
    inFile.get(id);
    //Incrementing frequency of characters that appear in the input file
    while (!inFile.eof()) {
        arr[id]->freq++;
        inFile.get(id);
    }
    inFile.close();
    //Pushing the Nodes which appear in the file into the priority queue (Min Heap)
    for (int i = 0; i < 128; i++) {
        if (arr[i]->freq > 0) {
            minHeap.push(arr[i]);
        }
    }
}

void huffman::createTree() {
    //Creating Huffman Tree with the Min Heap created earlier
    Node *left, *right;
    priority_queue <Node*, vector<Node*>, Compare> tempPQ(minHeap);
    while (tempPQ.size() != 1)
    {
        left = tempPQ.top();
        tempPQ.pop();
            
        right = tempPQ.top();
        tempPQ.pop();
            
        root = new Node();
        root->freq = left->freq + right->freq;

        root->left = left;
        root->right = right;
        tempPQ.push(root);
    }
}

void huffman::createCodes() {
    //Traversing the Huffman Tree and assigning specific codes to each character
    traverse(root, "");
}

void huffman::saveEncodedFile() {
    //Saving encoded (.huf) file
    inFile.open(inFileName, ios::in);
    outFile.open(outFileName, ios::out | ios::binary);
    string in = "";
    string s = "";
    char id;

    //Saving the meta data (huffman tree)
    in += (char)minHeap.size();
    priority_queue <Node*, vector<Node*>, Compare> tempPQ(minHeap);
    while (!tempPQ.empty()) {
        Node* curr = tempPQ.top();
        in += curr->data;
        //Saving 16 decimal values representing code of curr->data
        s.assign(127 - curr->code.length(), '0');
        s += '1';
        s += curr->code;
        //Saving decimal values of every 8-bit binary code 
        in += (char)binToDec(s.substr(0, 8));
        for (int i = 0; i < 15; i++) {
            s = s.substr(8);
            in += (char)binToDec(s.substr(0, 8));
        }
        tempPQ.pop();
    }
    s.clear();

    //Saving codes of every charachter appearing in the input file
    inFile.get(id);
    while (!inFile.eof()) {
        s += arr[id]->code;
        //Saving decimal values of every 8-bit binary code
        while (s.length() > 8) {
            in += (char)binToDec(s.substr(0, 8));
            s = s.substr(8);
        }
        inFile.get(id);
    }

    //Finally if bits remaining are less than 8, append 0's
    int count = 8 - s.length();
	if (s.length() < 8) {
		s.append(count, '0');
	}
	in += (char)binToDec(s);	
    //append count of appended 0's
    in += (char)count;

    //write the in string to the output file
	outFile.write(in.c_str(), in.size());
	inFile.close();
	outFile.close();
}

void huffman::saveDecodedFile() {
    inFile.open(inFileName, ios::in | ios::binary);
    outFile.open(outFileName, ios::out);
    unsigned char size;
    inFile.read(reinterpret_cast<char*>(&size), 1);
    //Reading count at the end of the file which is number of bits appended to make final value 8-bit
    inFile.seekg(-1, ios::end);
    char count0;
    inFile.read(&count0, 1);
    //Ignoring the meta data (huffman tree) (1 + 17 * size) and reading remaining file
    inFile.seekg(1 + 17 * size, ios::beg);

    vector<unsigned char> text;
    unsigned char textseg;
    inFile.read(reinterpret_cast<char*>(&textseg), 1);
    while (!inFile.eof()) {
        text.push_back(textseg);
        inFile.read(reinterpret_cast<char*>(&textseg), 1);
    }

    Node *curr = root;
    string path;
    for (int i = 0; i < (int)text.size() - 1; i++) {
        //Converting decimal number to its equivalent 8-bit binary code
        path = decToBin(text[i]);
        if (i == text.size() - 2) {
            path = path.substr(0, 8 - count0);
        }
        //Traversing huffman tree and appending resultant data to the file
        for (int j = 0; j <(int)path.size(); j++) {
            if (path[j] == '0') {
                curr = curr->left;
            }
            else {
                curr = curr->right;
            }

            if (curr->left == NULL && curr->right == NULL) {
                outFile.put(curr->data);
                curr = root;
            }
        }
    }
    inFile.close();
    outFile.close();
}

void huffman::getTree() {
    inFile.open(inFileName, ios::in | ios::binary);
    //Reading size of MinHeap
    unsigned char size;
    inFile.read(reinterpret_cast<char*>(&size), 1);
    root = new Node();
    //next size * (1 + 16) characters contain (char)data and (string)code[in decimal]
    for(int i = 0; i < size; i++) {
        char aCode;
        unsigned char hCodeC[16];
        inFile.read(&aCode, 1);
        inFile.read(reinterpret_cast<char*>(hCodeC), 16);
        //converting decimal characters into their binary equivalent to obtain code
        string hCodeStr = "";
        for (int i = 0; i < 16; i++) {
            hCodeStr += decToBin(hCodeC[i]);
        }
        //Removing padding by ignoring first (127 - curr->code.length()) '0's and next '1' character
        int j = 0;
        while (hCodeStr[j] == '0') {
            j++;
        }
        hCodeStr = hCodeStr.substr(j+1);
        //Adding node with aCode data and hCodeStr string to the huffman tree
        buildTree(aCode, hCodeStr);
    }
    inFile.close();
}

void huffman::compress() {
    createMinHeap();
    createTree();
    createCodes();
    saveEncodedFile();
}

void huffman::decompress() {
    getTree();
    saveDecodedFile();
}

//SFML IMPLEMENTATION

int main()
{
    std::cout << "LOADING SFML.......";
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Huffman Demo!");
    sf::CircleShape shape(100.f);
    sf::RectangleShape rectc(sf::Vector2f(300.f, 100.f)), rectdc(sf::Vector2f(300.f, 100.f));
    rectc.setFillColor(sf::Color::Green);
    rectc.setPosition(500.f, 350.f);
    rectdc.setFillColor(sf::Color::Green);
    rectdc.setPosition(500.f, 500.f);
    sf::Clock c;
    sf::Time elapsed = c.getElapsedTime();
   string stri;

    sf::Font fonts;
    fonts.loadFromFile("C:\\Users\\panth\\OneDrive\\Desktop\\Project\\Project1\\Fonts\\JosefinSans-Light.ttf");
    sf::Text Compress, Decompress, head1, head2, head3,chead,dhead,by;
    Compress.setFont(fonts); chead.setFont(fonts);
    Compress.setString("Compress"); chead.setString("Enter file to compress.\n: ");
    Compress.setPosition(570.f, 370.f);
    Compress.setFillColor(sf::Color::Black);
    Compress.setCharacterSize(40); chead.setCharacterSize(40);
    Compress.setStyle(sf::Text::Bold); chead.setStyle(sf::Text::Bold);

    Decompress.setFont(fonts); dhead.setFont(fonts);
    Decompress.setString("Decompress"); dhead.setString("Enter file to decompress.\n: ");
    Decompress.setPosition(550.f, 525.f);
    Decompress.setFillColor(sf::Color::Black);
    Decompress.setCharacterSize(40); dhead.setCharacterSize(40);
    Decompress.setStyle(sf::Text::Bold);

    head1.setFont(fonts); head2.setFont(fonts); head3.setFont(fonts);
    head1.setString("FILE COMPRESSION"); head2.setString("Using"); head3.setString("HUFFMAN CODING");
    head1.setPosition(500.f, 80.f); head2.setPosition(600.f, 130.f); head3.setPosition(350.f, 200.f); 
    head3.setCharacterSize(60);
    head1.setStyle(sf::Text::Bold); head3.setStyle(sf::Text::Italic | sf::Text::Bold);

    by.setFont(fonts);
    by.setString("By:\nNischal Panthi\nNeeka Maharjan\nPunam Adhikari");
    by.setPosition(0.f, 600.f);
    by.setStyle(sf::Text::Bold);


    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

        }
      
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (mousePos.x > 500 and mousePos.x < 800 and mousePos.y>350 and mousePos.y < 450) {
                sf::RenderWindow encode(sf::VideoMode(640, 200), "Compress");
                while (encode.isOpen()) {
                    sf::Event comevt;
                    while (encode.pollEvent(comevt))
                    {
                        if (comevt.type == sf::Event::Closed)
                            encode.close();
                    }
                    sf::Time elapsed = c.getElapsedTime();
                    if (comevt.type == sf::Event::TextEntered and elapsed.asSeconds() > 0.1) {
                        rectc.setFillColor(sf::Color::Red);
                        if (comevt.text.unicode != 8 and comevt.text.unicode != 13) {
                            stri = stri + (char)(comevt.text.unicode);
                            chead.setString("Enter file to compress.\n: " + stri);
                        }
                        else if (comevt.text.unicode == 8) {
                            stri = stri.substr(0, stri.length() - 1);
                            chead.setString("Enter file to compress.\n: " + stri);
                        }
                        else if (comevt.text.unicode == 13)
                        {
                            huffman f("Test\\" + stri + ".txt", "Test\\Compressed_" + stri + ".txt");
                            f.compress();
                            chead.setString("Enter the file to compress.\n: " + stri + "\n\n\t\t\tSUCCESS");
                        }

                        c.restart();

                    }

                    encode.clear();
                    encode.draw(chead);
                    encode.display();

                }

            }
        }
         if(sf::Mouse::isButtonPressed(sf::Mouse::Left)){
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (mousePos.x > 500 and mousePos.x < 800 and mousePos.y>500 and mousePos.y < 600) {
                   sf::RenderWindow decode(sf::VideoMode(640, 200), "Decompress");
                    while (decode.isOpen()) {
                        sf::Event decomevt;
                        while (decode.pollEvent(decomevt))
                        {
                            if (decomevt.type == sf::Event::Closed)
                                decode.close();
                        }
                         sf::Time elapsed = c.getElapsedTime();
                         if (decomevt.type == sf::Event::TextEntered and elapsed.asSeconds() > 0.1) {
                             if (decomevt.text.unicode != 8 and decomevt.text.unicode != 13) {
                                 stri = stri + (char)(decomevt.text.unicode);
                                 dhead.setString("Enter file to decompress.\n: " + stri);
                             }
                             else if (decomevt.text.unicode == 8) {
                                 stri = stri.substr(0, stri.length() - 1);
                                 dhead.setString("Enter file to decompress.\n: " + stri);
                             }
                             else if (decomevt.text.unicode == 13)
                             {
                                 huffman f("Test\\" + stri+".txt", "Test\\Decompressed" + stri.substr(10, stri.length())+".txt");
                                 f.decompress();
                                 dhead.setString("Enter the file to decompress.\n: " + stri + "\n\n\t\t\tSUCCESS");
                             }
                             c.restart();
                         }


                        decode.clear();
                        decode.draw(dhead);
                        decode.display();

            }
              }
          }
       window.clear();

            window.draw(rectc);
            window.draw(Compress);
            window.draw(rectdc);
            window.draw(Decompress);
            window.draw(head1); window.draw(head2); window.draw(head3);
            window.draw(by);
        window.display();
    }

    return 0;
}